#include <memory>
#include <iostream>
#include <fstream>
#include <chrono>
#include "tclap/CmdLine.h"

#include "cola/parse.hpp"
#include "types/guarantees.hpp"
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "cola/util.hpp"

#include "types/preprocess.hpp"
#include "types/rmraces.hpp"
#include "types/check.hpp"
#include "types/cave.hpp"
#include "types/synthesis.hpp"

// #include "types/observer.hpp"
// #include "types/factory.hpp"

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
	std::string description() const override { return "path to regular file"; }
 	std::string shortID() const override { return "path"; }
 	bool check(const std::string& path) const override {
 		std::ifstream stream(path.c_str());
    	return stream.good();
 	}
};

static void fail_if(bool condition, CmdLine& cmd, std::string description="undefined exception", std::string id="undefined") {
	if (condition) {
		SpecificationException error(description, id);
		cmd.getOutput()->failure(cmd, error);
	}
}


struct LeapConfig {
	std::string program_path, observer_path, output_path;
	bool check_types, check_annotations, check_linearizability;
	bool rewrite_and_retry;
	bool interactive, eager;
	bool quiet, verbose;
	bool output;
	bool synthesize_types;
} config;

struct ParseUnit {
	std::shared_ptr<Program> program;
	std::unique_ptr<SmrObserverStore> store;
	std::unique_ptr<GuaranteeTable> table;
} input;

enum AnalysisResult { SAFE, FAIL, UDEF };

struct AnalysisOutput {
	AnalysisResult type_safe = UDEF;
	AnalysisResult annotations_hold = UDEF;
	AnalysisResult linearizable = UDEF;
	duration_t time_synthesis = ZERO_DURATION;
	duration_t time_types = ZERO_DURATION;
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

// static void read_input() { // HP
// 	// parse program
// 	input.program = cola::parse(config.program_path);
// 	Program& program = *input.program;

// 	// query HP functions
// 	auto search_retire = find_function(program, "retire");
// 	auto search_protect1 = find_function(program, "protect1");
// 	auto search_protect2 = find_function(program, "protect2");
// 	assert(search_retire.has_value());
// 	assert(search_protect1.has_value());
// 	assert(search_protect2.has_value());
// 	const Function& retire = *search_retire;
// 	const Function& protect1 = *search_protect1;
// 	const Function& protect2 = *search_protect2;
	
// 	// preprocess program
// 	std::cout << std::endl << "Preprocessing program... " << std::flush;
// 	prtypes::preprocess(program, retire);
// 	program.name += " (preprocessed)";
// 	std::cout << "done" << std::endl;
// 	std::cout << "Preprocessed program: " << std::endl;
// 	cola::print(program, std::cout);

// 	// create SMR observers
// 	// TODO: parse SMR impl observers
// 	std::cout << std::endl << "Preparing SMR observers... " << std::flush;
// 	input.store = std::make_unique<SmrObserverStore>(program, retire);
// 	SmrObserverStore& store = *input.store;
// 	// store.add_impl_observer(make_hp_no_transfer_observer(retire, protect1));
// 	// store.add_impl_observer(make_hp_no_transfer_observer(retire, protect2));
// 	store.add_impl_observer(make_hp_transfer_observer(retire, protect1, protect2));
// 	std::cout << "done" << std::endl;
// 	std::cout << "The SMR observer is the cross-product of: " << std::endl;
// 	cola::print(*store.base_observer, std::cout);
// 	for (const auto& observer : store.impl_observer) {
// 		cola::print(*observer, std::cout);
// 	}


// 	if (!config.check_types) {
// 		return;
// 	}

// 	// create guarantee table
// 	input.table = std::make_unique<GuaranteeTable>(store);
// 	GuaranteeTable& table = *input.table;
	
// 	if (config.synthesize_types) {
// 		std::cout << std::endl << "Synthesizing types... " << std::flush;
// 		timepoint_t begin = get_time();
// 		prtypes::populate_guarantee_table_with_synthesized_guarantees(table);
// 		output.time_synthesis = get_elapsed(begin);
// 		std::cout << "done" << std::endl;

// 	} else {
// 		std::cout << std::endl << "Loading custom types... " << std::flush;
// 		auto add_guarantees = [&](const Function& func, const Function* trans=nullptr) {
// 			std::vector<std::unique_ptr<cola::Observer>> hp_guarantee_observers;
// 			if (trans) {
// 				hp_guarantee_observers = prtypes::make_hp_transfer_guarantee_observers(retire, func, *trans, func.name);
// 			} else {
// 				hp_guarantee_observers = prtypes::make_hp_no_transfer_guarantee_observers(retire, func, func.name);
// 			}
// 			table.add_guarantee(std::move(hp_guarantee_observers.at(0)), "E1-" + func.name);
// 			table.add_guarantee(std::move(hp_guarantee_observers.at(1)), "E2-" + func.name);
// 			table.add_guarantee(std::move(hp_guarantee_observers.at(2)), "P-" + func.name);
// 			std::cout << "done" << std::endl;
// 			if (hp_guarantee_observers.size() > 3) {
// 				assert(hp_guarantee_observers.size() == 5);
// 				table.add_guarantee(std::move(hp_guarantee_observers.at(3)), "Et-" + func.name);
// 				table.add_guarantee(std::move(hp_guarantee_observers.at(4)), "Pt-" + func.name);
// 			}
// 		};
// 		add_guarantees(protect1, &protect2);
// 		add_guarantees(protect2, &protect1);
// 	}

// 	if (config.verbose) {
// 		std::cout << std::endl << "List of guarantees "  << table.all_guarantees.size() <<  ":" << std::endl;
// 		for (const auto& guarantee : table) {
// 			std::cout << "  - " << "(transient, valid) = (" << guarantee.is_transient << ", " << guarantee.entails_validity << ")  for  " << guarantee.name << std::endl;
// 		}
// 	} else if (!config.quiet) {
// 		std::cout << "Using " << table.all_guarantees.size() << " guarantees in the type system." << std::endl;
// 	}
// }

static void read_input() {
	// parse program
	input.program = cola::parse(config.program_path);
	Program& program = *input.program;

	// query HP functions
	auto search_retire = find_function(program, "retire");
	auto search_enter = find_function(program, "enterQ");
	auto search_leave = find_function(program, "leaveQ");
	assert(search_retire.has_value());
	assert(search_enter.has_value());
	assert(search_leave.has_value());
	const Function& retire = *search_retire;
	const Function& enter = *search_enter;
	const Function& leave = *search_leave;
	
	// preprocess program
	std::cout << std::endl << "Preprocessing program... " << std::flush;
	prtypes::preprocess(program, retire);
	program.name += " (preprocessed)";
	std::cout << "done" << std::endl;
	std::cout << "Preprocessed program: " << std::endl;
	cola::print(program, std::cout);

	// create SMR observers
	// TODO: parse SMR impl observers
	std::cout << std::endl << "Preparing SMR observers... " << std::flush;
	input.store = std::make_unique<SmrObserverStore>(program, retire);
	SmrObserverStore& store = *input.store;
	store.add_impl_observer(make_ebr_observer(retire, enter, leave));
	std::cout << "done" << std::endl;
	std::cout << "The SMR observer is the cross-product of: " << std::endl;
	cola::print(*store.base_observer, std::cout);
	for (const auto& observer : store.impl_observer) {
		cola::print(*observer, std::cout);
	}


	if (!config.check_types) {
		return;
	}

	// create guarantee table
	input.table = std::make_unique<GuaranteeTable>(store);
	GuaranteeTable& table = *input.table;
	
	if (config.synthesize_types) {
		std::cout << std::endl << "Synthesizing types... " << std::flush;
		timepoint_t begin = get_time();
		prtypes::populate_guarantee_table_with_synthesized_guarantees(table);
		output.time_synthesis = get_elapsed(begin);
		std::cout << "done" << std::endl;

	} else {
		throw std::logic_error("not yet implemented");
	}

	if (config.verbose) {
		std::cout << std::endl << "List of guarantees "  << table.all_guarantees.size() <<  ":" << std::endl;
		for (const auto& guarantee : table) {
			std::cout << "  - " << "(transient, valid) = (" << guarantee.is_transient << ", " << guarantee.entails_validity << ")  for  " << guarantee.name << std::endl;
		}
	} else if (!config.quiet) {
		std::cout << "Using " << table.all_guarantees.size() << " guarantees in the type system." << std::endl;
	}
}

template<typename ErrorClass, typename... Targs>
void try_fix(ErrorClass& err, Targs... args) {
	if (!config.rewrite_and_retry) {
		// throw err;
		std::cout << "Type error at statement: ";
		cola::print(err.pc, std::cout);
		std::cout << err.what() << std::endl;
	} else {
		output.number_rewrites++;
		auto begin = get_time();
		prtypes::try_fix_pointer_race(*input.program, *input.table, err, args...);
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
		try {
			auto begin = get_time();
			type_safe = prtypes::type_check(*input.program, *input.table);
			output.time_types += get_elapsed(begin);

		} catch (UnsafeCallError err) {
			try_fix(err);

		} catch (UnsafeDereferenceError err) {
			try_fix(err);

		} catch (UnsafeAssumeError err) {
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
	assertions_safe = discharge_assertions(*input.program, *input.table);
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

static void print_gist() {
	if (config.quiet) {
		return;
	}

	auto gist_synthesis = [&]() -> std::string {
		if (config.synthesize_types) {
			return std::to_string(input.table->all_guarantees.size()) + " types in " + to_s(output.time_synthesis);
		} else {
			return "--skipped--";
		}
	};
	auto gist_rewrite = [&]() -> std::string {
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
	auto gist_types = [&]() -> std::string {
		if (config.check_types) {
			std::string verdict = output.type_safe == SAFE ? "successful" : "failed";
			return verdict + " after " + to_s(output.time_types);
		} else {
			return "--skipped--";
		}
	};
	auto gist_annotations = [&]() -> std::string {
		if (config.check_annotations && output.type_safe != FAIL) {
			std::string verdict = output.annotations_hold == SAFE ? "successful" : "failed";
			return verdict + " after " + to_s(output.time_annotations);
		} else {
			return "--skipped--";
		}
	};
	auto gist_linearizability  = [&]() -> std::string {
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
	std::cout << "# Type Synthesis:         " << gist_synthesis() << std::endl;
	std::cout << "# Type Check:             " << gist_types() << std::endl;
	std::cout << "# Rewrites:               " << gist_rewrite() << std::endl;
	std::cout << "# Annotation Check:       " << gist_annotations() << std::endl;
	std::cout << "# Linearizability Check:  " << gist_linearizability() << std::endl;
}

int main(int argc, char** argv) {

	// parse command line arguments
	try {
		CmdLine cmd("LEAP verification tool for lock-free data structures", ' ', "0.9");
		auto is_file_constraint = std::make_unique<IsRegularFileConstraint>();

		// path to file containing observer definition
		// static const std::string NO_OBSERVER_PATH = "<none>";
		// ValueArg<std::string> observer_arg("o", "observer", "Observer specification for SMR algorithm", false , NO_OBSERVER_PATH, "path", cmd);
		// SwitchArg rewrite_switch("r", "rewrite", "Rewrite program and retry type check upon type errors", cmd, false);
		SwitchArg keep_switch("s", "simple", "Simple type check; no rewriting&retrying (atomicity abstraction) upon type errors", cmd, false);
		SwitchArg customtype_switch("c", "customtypes", "Do not synthesize typse, but use custom types", cmd, false);
		SwitchArg type_switch("t", "checktypes", "Perform type check", cmd, false);
		SwitchArg annotation_switch("a", "checkannotations", "Perform annotation check", cmd, false);
		SwitchArg linearizability_switch("l", "checklinearizability", "Perform linearizability check", cmd, false);
		SwitchArg interactive_switch("i", "interactive", "Interactive mode to control type check", cmd, false);
		SwitchArg eager_switch("e", "eager", "Eagerly checks annotations before adding them", cmd, false);
		SwitchArg quiet_switch("q", "quiet", "Disables most output", cmd, false);
		SwitchArg verbose_switch("v", "verbose", "Verbose output", cmd, false);
		ValueArg<std::string> output_arg("o", "output", "Output file for transformed program", false , "", "path", cmd);
		UnlabeledValueArg<std::string> program_arg("program", "Input program file to analyse", true, "", is_file_constraint.get(), cmd);

		cmd.parse( argc, argv );
		config.program_path = program_arg.getValue();
		config.check_types = type_switch.getValue();
		config.rewrite_and_retry = !keep_switch.getValue();
		config.synthesize_types = !customtype_switch.getValue();
		config.check_annotations = annotation_switch.getValue();
		config.check_linearizability = linearizability_switch.getValue();
		config.interactive = interactive_switch.getValue();
		config.eager = eager_switch.getValue();
		config.quiet = quiet_switch.getValue();
		config.verbose = verbose_switch.getValue();
		config.output_path = output_arg.getValue();
		config.output = output_arg.isSet();

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

	print_gist();

	return 0;
}
