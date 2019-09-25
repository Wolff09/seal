#include <memory>
#include <iostream>
#include <fstream>
#include <chrono>
#include "tclap/CmdLine.h"

#include "cola/parse.hpp"
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "cola/util.hpp"

#include "types/preprocess.hpp"
#include "types/rmraces.hpp"
#include "types/check.hpp"
#include "types/cave.hpp"

using namespace TCLAP;
using namespace cola;
using namespace prtypes;

using timepoint_t = std::chrono::steady_clock::time_point;
using duration_t = std::chrono::milliseconds;

static const duration_t ZERO_DURATION = std::chrono::milliseconds(0);

timepoint_t get_time() {
	return std::chrono::steady_clock::now();
}

duration_t get_elapsed(timepoint_t since) {
	timepoint_t now = get_time();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now - since);
}

std::string to_ms(duration_t duration) {
	return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()) + "ms";
}

std::string to_s(duration_t duration) {
	auto count = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	auto seconds = count / 1000;
	auto milli = count - seconds;
	while (milli >= 100) {
		milli = milli / 10;
	}
	return std::to_string(seconds) + "." + std::to_string(milli) + "s";
}


struct IsRegularFileConstraint : public Constraint<std::string> {
	std::string id = "path";
	std::string description() const override { return "path to regular file"; }
 	std::string shortID() const override { return this->id; }
 	bool check(const std::string& path) const override {
 		std::ifstream stream(path.c_str());
    	return stream.good();
 	}
 	IsRegularFileConstraint(std::string more_verbose="") {
 		this->id += more_verbose;
 	}
};

static void fail_if(bool condition, CmdLine& cmd, std::string description="undefined exception", std::string id="undefined") {
	if (condition) {
		SpecificationException error(description, id);
		cmd.getOutput()->failure(cmd, error);
	}
}


struct LeapConfig {
	std::string program_path, observer_path, output_path, customtypes_path;
	bool check_types, check_annotations, check_linearizability;
	bool rewrite_and_retry;
	bool interactive, eager;
	bool quiet, verbose;
	bool print_gist;
	bool output;
} config;

enum SmrType { SMR_HP, SMR_EBR };

std::string smr_to_string(SmrType type) {
	switch(type) {
		case SMR_HP: return "HP";
		case SMR_EBR: return "EBR";
	}
}

struct ParseUnit {
	std::shared_ptr<Program> program;
	std::unique_ptr<SmrObserverStore> store;
} input;

enum AnalysisResult { SAFE, FAIL, UDEF };

struct AnalysisOutput {
	AnalysisResult type_safe = UDEF;
	AnalysisResult annotations_hold = UDEF;
	AnalysisResult linearizable = UDEF;
	duration_t time_types_last = ZERO_DURATION;
	duration_t time_types_total = ZERO_DURATION;
	duration_t time_rewrite = ZERO_DURATION;
	duration_t time_annotations = ZERO_DURATION;
	duration_t time_linearizability = ZERO_DURATION;
	std::size_t number_rewrites = 0;
} output;


inline std::optional<std::reference_wrapper<const Function>> find_function(const Program& program, std::string name) {
	for (const auto& function : program.functions) {
		if (function->name == name) {
			return *function;
		}
	}
	return std::nullopt;
}

inline const Function& find_function_or_fail(const Program& program, std::string name) {
	auto search = find_function(program, name);
	if (!search.has_value()) {
		std::cout << "I expected function '" << name << "' to exist, but it does not." << std::endl;
		throw std::logic_error("Failed to find function '" + name + "'.");
	}
	return *search;
}

inline std::pair<const Function&, const Function&> lookup_functions(const Program& program, std::string name1, std::string name2) {
	return { find_function_or_fail(program, name1), find_function_or_fail(program, name2) };
}

static void create_smr_observer(const Program& program, const Function& retire) {
	std::cout << std::endl << "Preparing SMR automaton..." << std::flush;
	auto observers = cola::parse_observer(config.observer_path, program);
	input.store = std::make_unique<SmrObserverStore>(program, retire);
	for (auto& observer : observers) {
		input.store->add_impl_observer(std::move(observer));
	}
	std::cout << "done" << std::endl;
	std::cout << "The SMR observer is the cross-product of (.dot): " << std::endl;
	cola::print(*input.store->base_observer, std::cout);
	for (const auto& observer : input.store->impl_observer) {
		cola::print(*observer, std::cout);
	}
}

static void read_input() {
	// parse program
	input.program = cola::parse_program(config.program_path);
	Program& program = *input.program;

	// query retire
	auto search_retire = find_function(program, "retire");
	assert(search_retire.has_value());
	const Function& retire = *search_retire;
	
	// preprocess program
	std::cout << std::endl << "Preprocessing program... " << std::flush;
	prtypes::preprocess(program, retire);
	program.name += " (preprocessed)";
	std::cout << "done" << std::endl;
	std::cout << "Preprocessed program: " << std::endl;
	cola::print(program, std::cout);

	// init SMR
	create_smr_observer(program, retire);
}

template<typename ErrorClass, typename... Targs>
void try_fix(ErrorClass& err, Targs... args) {
	cola::print(err.pc, std::cout);
	std::cout << err.what() << std::endl;

	if (!config.rewrite_and_retry) {
		// throw err;
		std::cout << "(I was told to not fix it.)" << std::endl;
	} else {
		output.number_rewrites++;
		auto begin = get_time();
		prtypes::try_fix_pointer_race(*input.program, *input.store, err, args...);
		output.time_rewrite += get_elapsed(begin);
	}
}

static void do_type_check() {
	std::unique_ptr<UnsafeAssumeError> previous_unsafe_assume_error;
	auto is_reoffending = [&](const UnsafeAssumeError& error) {
		if (!previous_unsafe_assume_error) {
			return false;
		} else {
			UnsafeAssumeError& other = *previous_unsafe_assume_error;
			return &error.pc == &other.pc && &error.var == &other.var;
		}
	};

	bool type_safe;
	do {
		std::cout << std::endl << "Checking typing..." << std::endl;
		auto begin = get_time();
		try {
			type_safe = prtypes::type_check(*input.program, *input.store);
			output.time_types_total += get_elapsed(begin);
			output.time_types_last = get_elapsed(begin);

		} catch (UnsafeCallError err) {
			output.time_types_total += get_elapsed(begin);
			output.time_types_last = get_elapsed(begin);
			try_fix(err);

		} catch (UnsafeDereferenceError err) {
			output.time_types_total += get_elapsed(begin);
			output.time_types_last = get_elapsed(begin);
			try_fix(err);

		} catch (UnsafeAssumeError err) {
			output.time_types_total += get_elapsed(begin);
			output.time_types_last = get_elapsed(begin);
			bool reoffending = is_reoffending(err);
			try_fix(err, reoffending);
		}

	} while (!type_safe && config.rewrite_and_retry);

	// print program after modifications
	if (config.rewrite_and_retry) {
		std::cout << "Transformed program: " << std::endl;
		input.program->name += " (transformed)";
		cola::print(*input.program, std::cout);
	}

	std::cout << "** Type check " << (type_safe ? "succeeded" : "failed") << " **" << std::endl << std::endl;
	if (type_safe) output.type_safe = SAFE;
	else output.type_safe = FAIL;
}

static void do_annotation_check() {
	std::cout << std::endl << "Checking assertions... " << std::flush;
	bool assertions_safe = false;
	auto begin = get_time();
	assertions_safe = discharge_assertions(*input.program, *input.store);
	output.time_annotations = get_elapsed(begin);
	std::cout << "done" << std::endl;
	std::cout << "** Assertion check: " << (assertions_safe ? "succeeded" : "failed") << " **" << std::endl << std::endl;
	if (assertions_safe) output.annotations_hold = SAFE;
	else output.annotations_hold = FAIL;
}

static void do_linearizability_check() {
	std::cout << std::endl << "Checking linearizability under GC... " << std::flush;
	auto begin = get_time();
	bool linearizable = prtypes::check_linearizability(*input.program);
	output.time_linearizability = get_elapsed(begin);
	std::cout << "done" << std::endl;
	std::cout << "** Linearizability check: " << (linearizable ? "succeeded" : "failed") << " **" << std::endl << std::endl;
	if (linearizable) output.linearizable = SAFE;
	else output.linearizable = FAIL;
}

static void print_summary() {
	if (config.quiet) {
		return;
	}

	auto summary_rewrite = [&]() -> std::string {
		if (config.rewrite_and_retry) {
			if (output.number_rewrites == 0) {
				return "none";
			} else {
				return std::to_string(output.number_rewrites) + " rewrites in " + to_s(output.time_rewrite);
			}
		} else {
			return "--skipped--";
		}
	};
	auto summary_types = [&]() -> std::string {
		if (config.check_types) {
			std::string verdict = output.type_safe == SAFE ? "successful" : "failed";
			std::string addition = output.number_rewrites == 0 ? "" : " (total " + to_s(output.time_types_total) + ")";
			return verdict + " after " + to_s(output.time_types_last) + addition;
		} else {
			return "--skipped--";
		}
	};
	auto summary_annotations = [&]() -> std::string {
		if (config.check_annotations && output.type_safe != FAIL) {
			std::string verdict = output.annotations_hold == SAFE ? "successful" : "failed";
			return verdict + " after " + to_s(output.time_annotations);
		} else {
			return "--skipped--";
		}
	};
	auto summary_linearizability  = [&]() -> std::string {
		if (config.check_linearizability && output.type_safe != FAIL && output.annotations_hold != FAIL) {
			std::string verdict = output.linearizable == SAFE ? "successful" : "failed";
			return verdict + " after " + to_s(output.time_linearizability);
		} else {
			return "--skipped--";
		}
	};

	std::cout << std::endl << std::endl;
	std::cout << "# Summary:" << std::endl;
	std::cout << "# ========" << std::endl;
	std::cout << "# Type Check:             " << summary_types() << std::endl;
	std::cout << "# Rewrites:               " << summary_rewrite() << std::endl;
	std::cout << "# Annotation Check:       " << summary_annotations() << std::endl;
	std::cout << "# Linearizability Check:  " << summary_linearizability() << std::endl;
}

void print_gist() {
	if (!config.print_gist) {
		return;
	}
	auto mk_status = [](bool enabled, AnalysisResult result, duration_t time) -> std::string {
		if (!enabled) {
			return "-:-";
		}
		std::string verdict;
		switch (result) {
			case SAFE: verdict = "1"; break;
			case FAIL: verdict = "0"; break;
			case UDEF: verdict = "?"; break;
		}
		return verdict + ":" + to_s(time);;
	};
	std::cout << std::endl << std::endl;
	std::cout << "#gist=";
	std::cout << mk_status(config.check_types, output.type_safe, output.time_types_last) << ";";
	std::cout << mk_status(config.check_annotations, output.annotations_hold, output.time_annotations) << ";";
	std::cout << mk_status(config.check_linearizability, output.linearizable, output.time_linearizability) << std::endl;
}

int main(int argc, char** argv) {

	// parse command line arguments
	try {
		CmdLine cmd("SEAL verification tool for lock-free data structures with safe memory reclamation", ' ', "0.9");
		auto is_program_constraint = std::make_unique<IsRegularFileConstraint>("_to_program");
		auto is_observer_constraint = std::make_unique<IsRegularFileConstraint>("_to_smr");

		// path to file containing observer definition
		// static const std::string NO_OBSERVER_PATH = "<none>";
		// ValueArg<std::string> observer_arg("o", "observer", "Observer specification for SMR algorithm", false , NO_OBSERVER_PATH, "path", cmd);
		// SwitchArg rewrite_switch("r", "rewrite", "Rewrite program and retry type check upon type errors", cmd, false);
		SwitchArg keep_switch("s", "simple", "Simple type check; no rewriting&retrying (atomicity abstraction) upon type errors", cmd, false);
		// SwitchArg customtype_switch("c", "customtypes", "Do not synthesize typse, but use custom types", cmd, false);
		SwitchArg type_switch("t", "checktypes", "Perform type check", cmd, false);
		SwitchArg annotation_switch("a", "checkannotations", "Perform annotation check", cmd, false);
		SwitchArg linearizability_switch("l", "checklinearizability", "Perform linearizability check", cmd, false);
		// SwitchArg interactive_switch("i", "interactive", "Interactive mode to control type check", cmd, false);
		SwitchArg eager_switch("e", "eager", "Eagerly checks annotations before adding them", cmd, false);
		// SwitchArg quiet_switch("q", "quiet", "Disables most output", cmd, false);
		// SwitchArg verbose_switch("v", "verbose", "Verbose output", cmd, false);
		SwitchArg gist_switch("g", "gist", "Print machine readable gist at the very end", cmd, false);
		// ValueArg<std::string> output_arg("o", "output", "Output file for transformed program", false , "", "path", cmd);
		UnlabeledValueArg<std::string> program_arg("program", "Input program file to analyze", true, "", is_program_constraint.get(), cmd);
		UnlabeledValueArg<std::string> observer_arg("observer", "Input observer file for SMR specification", true, "", is_observer_constraint.get(), cmd);

		cmd.parse( argc, argv );
		config.program_path = program_arg.getValue();
		config.observer_path = observer_arg.getValue();
		config.check_types = type_switch.getValue();
		config.rewrite_and_retry = !keep_switch.getValue();
		config.check_annotations = annotation_switch.getValue();
		config.check_linearizability = linearizability_switch.getValue();
		config.eager = eager_switch.getValue();
		config.print_gist = gist_switch.getValue();
		config.interactive = false;
		config.quiet = false;
		config.verbose = false;
		config.output = false;
		config.output_path = "";
		// config.interactive = interactive_switch.getValue();
		// config.quiet = quiet_switch.getValue();
		// config.verbose = verbose_switch.getValue();
		// config.output = output_arg.isSet();
		// config.output_path = output_arg.getValue();

		if (!config.check_types && !config.check_annotations && !config.check_linearizability) {
			config.check_types = config.check_annotations = config.check_linearizability = true;
		}

		// sanity checks
		fail_if(config.quiet && config.verbose, cmd, "Quiet and verbose mode cannot be used together.");
		fail_if(!config.check_types && config.interactive, cmd, "Interactive mode requires enabled type check.");
		// fail_if(!config.check_types && config.rewrite_and_retry, cmd, "Rewriting requires enabled type check.");
		fail_if(!config.rewrite_and_retry && config.eager, cmd, "Eager mode requires enabled rewriting.", "eager");
		fail_if(config.interactive && config.eager, cmd, "Eager and interactive mode cannot be used together.", "interactive");

	} catch (ArgException &e) {
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}
	// end: parse command line arguments


	// TODO: implement proper output facade respecting the flags
	if (config.verbose) { throw std::logic_error("Verbose mode not yet implemented"); }
	if (config.quiet) { throw std::logic_error("Quiet mode not yet implemented"); }

	// TODO: implement interactive mode
	if (config.interactive) { throw std::logic_error("Interactive mode not yet implemented"); }

	// TODO: implement output mode
	if (config.output) { throw std::logic_error("Output not yet implemented"); }


	// parse program, observer
	read_input();

	// do type check
	if (config.check_types) {
		do_type_check();
	}

	// check annotations
	if (config.check_annotations && output.type_safe != FAIL) {
		do_annotation_check();
	}

	// check linearizability
	if (config.check_linearizability && output.type_safe != FAIL && output.annotations_hold != FAIL) {
		do_linearizability_check();
	}

	print_summary();
	print_gist();
	return 0;
}
