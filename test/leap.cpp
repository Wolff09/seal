#include <memory>
#include <iostream>
#include <fstream>
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
} output;


inline std::optional<std::reference_wrapper<const Function>> find_function(const Program& program, std::string name) {
	for (const auto& function : program.functions) {
		if (function->name == name) {
			return *function;
		}
	}
	return std::nullopt;
}

template<typename T>
inline std::vector<T> to_vec(T obj) {
	std::vector<T> result;
	result.push_back(std::move(obj));
	return result;
}

static void read_input() {
	input.program = cola::parse(config.program_path);
	Program& program = *input.program;

	// query HP functions
	auto search_retire = find_function(program, "retire");
	auto search_protect1 = find_function(program, "protect1");
	auto search_protect2 = find_function(program, "protect2");
	assert(search_protect1.has_value());
	assert(search_protect2.has_value());
	const Function& retire = *search_retire;
	const Function& protect1 = *search_protect1;
	const Function& protect2 = *search_protect2;
	
	// preprocessing
	std::cout << std::endl << "Preprocessing program... " << std::flush;
	prtypes::preprocess(program, retire);
	program.name += " (preprocessed)";
	std::cout << "done" << std::endl;
	std::cout << "Preprocessed program: " << std::endl;
	cola::print(program, std::cout);

	// TODO: parse observer

	// create stuff
	std::cout << std::endl << "Computing simulation... " << std::flush;
	input.store = std::make_unique<SmrObserverStore>(program, retire);
	SmrObserverStore& store = *input.store;
	// store.add_impl_observer(make_hp_no_transfer_observer(retire, protect1));
	// store.add_impl_observer(make_hp_transfer_observer(retire, protect1, protect2)); // TODO: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<======================================================================== re-enable this
	store.add_impl_observer(make_hp_no_transfer_observer(retire, protect2));
	input.table = std::make_unique<GuaranteeTable>(store);
	GuaranteeTable& table = *input.table;
	std::cout << "done" << std::endl;

	// TODO: stop here if no type check is done?

//	// adding guarantees
//	auto add_guarantees = [&](const Function& func, const Function* trans=nullptr) {
//		std::cout << std::endl << "Adding guarantees for " << func.name << "... " << std::flush;
//		std::vector<std::unique_ptr<cola::Observer>> hp_guarantee_observers;
//		if (trans) {
//			hp_guarantee_observers = prtypes::make_hp_transfer_guarantee_observers(retire, func, *trans, func.name);
//		} else {
//			hp_guarantee_observers = prtypes::make_hp_no_transfer_guarantee_observers(retire, func, func.name);
//		}
//		table.add_guarantee(to_vec(std::move(hp_guarantee_observers.at(0))), "E1-" + func.name);
//		table.add_guarantee(to_vec(std::move(hp_guarantee_observers.at(1))), "E2-" + func.name);
//		table.add_guarantee(to_vec(std::move(hp_guarantee_observers.at(2))), "P-" + func.name);
//		std::cout << "done" << std::endl;
//		if (hp_guarantee_observers.size() > 3) {
//			assert(hp_guarantee_observers.size() == 5);
//			table.add_guarantee(to_vec(std::move(hp_guarantee_observers.at(3))), "Et-" + func.name);
//			table.add_guarantee(to_vec(std::move(hp_guarantee_observers.at(4))), "Pt-" + func.name);
//			// assert(guarantee_t.entails_validity);
//		}
//	};
//	add_guarantees(protect1, &protect2);
//	add_guarantees(protect2);
//
//	std::cout << "List of guarantees:" << std::endl;
//	for (const auto& guarantee : table) {
//		std::cout << "  - " << guarantee.name << ": (transient, valid) = (" << guarantee.is_transient << ", " << guarantee.entails_validity << ")" << std::endl;
//	}

	prtypes::populate_guarantee_table_with_synthesized_guarantees(table);

	std::cout << "List of guarantees:" << std::endl;
	for (const auto& guarantee : table) {
		std::cout << "  - " << guarantee.name << ": (transient, valid) = (" << guarantee.is_transient << ", " << guarantee.entails_validity << ")" << std::endl;
	}

	throw std::logic_error("---tmp break---");
}

template<typename ErrorClass, typename... Targs>
void try_fix(ErrorClass& err, Targs... args) {
	if (!config.rewrite_and_retry) {
		// throw err;
		std::cout << "Type error at statement: ";
		cola::print(err.pc, std::cout);
		std::cout << err.what() << std::endl;
	} else {
		try_fix_pointer_race(*input.program, *input.table, err, args...);
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
			type_safe = type_check(*input.program, *input.table);

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
	assertions_safe = discharge_assertions(*input.program, *input.table);
	std::cout << "done" << std::endl;
	std::cout << "** Assertion check: " << (assertions_safe ? "succeeded" : "failed") << " **" << std::endl << std::endl;
	if (assertions_safe) output.annotations_hold = SAFE;
	else output.annotations_hold = FAIL;
}

static void do_linearizability_check() {
	std::cout << std::endl << "Checking linearizability under GC... " << std::flush;
	bool linearizable = prtypes::check_linearizability(*input.program);
	std::cout << "done" << std::endl;
	std::cout << "** Linearizability check: " << (linearizable ? "succeeded" : "failed") << " **" << std::endl << std::endl;
	if (linearizable) output.linearizable = SAFE;
	else output.linearizable = FAIL;
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

	return 0;
}
