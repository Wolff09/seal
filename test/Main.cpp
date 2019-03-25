#include <iostream>
#include "types/check.hpp"
#include "types/observer.hpp"
#include "types/factory.hpp"
#include "types/guarantees.hpp"
#include "types/cave.hpp"
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "cola/parse.hpp"
#include "cola/util.hpp"
#include "types/rmraces.hpp"
#include "types/preprocess.hpp"

using namespace cola;
using namespace prtypes;


inline std::optional<std::reference_wrapper<const Function>> find_function(const Program& program, std::string name) {
	for (const auto& function : program.functions) {
		if (function->name == name) {
			return *function;
		}
	}
	return std::nullopt;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "No input file given." << std::endl;
		return -1;
	}

	auto filename = argv[1];
	auto program_ptr = cola::parse(filename);
	Program& program = *program_ptr;
	
	// std::cout << "Parsed program: " << std::endl;
	// cola::print(program, std::cout);

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
	std::cout << "done" << std::endl;
	program.name += " (preprocessed)";
	std::cout << "Preprocessed program: " << std::endl;
	cola::print(program, std::cout);

	// create stuff
	std::cout << std::endl << "Computing simulation... " << std::flush;
	SmrObserverStore store(program, retire);
	// store.add_impl_observer(make_hp_no_transfer_observer(retire, protect1));
	store.add_impl_observer(make_hp_transfer_observer(retire, protect1, protect2));
	store.add_impl_observer(make_hp_no_transfer_observer(retire, protect2));
	GuaranteeTable table(store);
	std::cout << "done" << std::endl;

	// adding guarantees
	auto add_guarantees = [&](const Function& func, const Function* trans=nullptr) {
		std::cout << std::endl << "Adding guarantees for " << func.name << "... " << std::flush;
		std::vector<std::unique_ptr<cola::Observer>> hp_guarantee_observers;
		if (trans) {
			hp_guarantee_observers = prtypes::make_hp_transfer_guarantee_observers(retire, func, *trans, func.name);
		} else {
			hp_guarantee_observers = prtypes::make_hp_no_transfer_guarantee_observers(retire, func, func.name);
		}
		auto& guarantee_e1 = table.add_guarantee(std::move(hp_guarantee_observers.at(0)), "E1-" + func.name);
		auto& guarantee_e2 = table.add_guarantee(std::move(hp_guarantee_observers.at(1)), "E2-" + func.name);
		auto& guarantee_p = table.add_guarantee(std::move(hp_guarantee_observers.at(2)), "P-" + func.name);
		std::cout << "done" << std::endl;
		std::cout << "  - " << guarantee_e1.name << ": (transient, valid) = (" << guarantee_e1.is_transient << ", " << guarantee_e1.entails_validity << ")" << std::endl;
		std::cout << "  - " << guarantee_e2.name << ": (transient, valid) = (" << guarantee_e2.is_transient << ", " << guarantee_e2.entails_validity << ")" << std::endl;
		std::cout << "  -  " << guarantee_p.name << ": (transient, valid) = (" << guarantee_p.is_transient << ", " << guarantee_p.entails_validity << ")" << std::endl;
		if (hp_guarantee_observers.size() > 3) {
			assert(hp_guarantee_observers.size() == 5);
			auto& guarantee_tprep = table.add_guarantee(std::move(hp_guarantee_observers.at(3)), "Et-" + func.name);
			std::cout << "  -  " << guarantee_tprep.name << ": (transient, valid) = (" << guarantee_tprep.is_transient << ", " << guarantee_tprep.entails_validity << ")" << std::endl;
			// std::cout << std::endl << std::endl << "****************************" << std::endl;
			auto& guarantee_t = table.add_guarantee(std::move(hp_guarantee_observers.at(4)), "Pt-" + func.name);
			std::cout << "  -  " << guarantee_t.name << ": (transient, valid) = (" << guarantee_t.is_transient << ", " << guarantee_t.entails_validity << ")" << std::endl;
			assert(guarantee_t.entails_validity);
		}
	};
	add_guarantees(protect1, &protect2);
	add_guarantees(protect2);
	// assert(false);

	// assertion check (initial ones given by programmer)
	if (true) {
		std::cout << std::endl << "Not checking initial assertions in program for assertion failures." << std::endl;
	} else {
		std::cout << std::endl << "Checking initial assertions..." << std::endl;
		bool initial_assertions_safe = discharge_assertions(program, table);
		std::cout << "Assertion check: " << (initial_assertions_safe ? "successful" : "failed") << std::endl;
	}

	// type check
	bool type_safe = false;
	std::unique_ptr<UnsafeAssumeError> previous_unsafe_assume_error;
	auto is_reoffending = [&](const UnsafeAssumeError& error) {
		if (!previous_unsafe_assume_error) {
			return false;
		} else {
			UnsafeAssumeError& other = *previous_unsafe_assume_error;
			return &error.pc == &other.pc && &error.var == &other.var;
		}
	};
	auto fix = [&](PointerRaceError& err) {
		std::cout << err.what() << std::endl;
		switch (err.kind()) {
			case PointerRaceError::CALL:
				try_fix_pointer_race(program, table, static_cast<UnsafeCallError&>(err));
				break;
			
			case PointerRaceError::DEREF:
				try_fix_pointer_race(program, table, static_cast<UnsafeDereferenceError&>(err));
				break;
			
			case PointerRaceError::ASSUME: {
				auto& error = static_cast<UnsafeAssumeError&>(err);
				bool reoffending = is_reoffending(error);
				try_fix_pointer_race(program, table, error, reoffending);
				break;
			}
		}
	};
	while (!type_safe) {
		std::cout << std::endl << "Checking typing..." << std::endl;
		try {
			type_safe = type_check(program, table);

		} catch (UnsafeCallError err) {
			fix(err);
		} catch (UnsafeDereferenceError err) {
			fix(err);
		} catch (UnsafeAssumeError err) {
			fix(err);
		}

		if (!type_safe) {
			cola::print(program, std::cout);
			throw std::logic_error("temp break");
		}
	}
	std::cout << "Type check " << (type_safe ? "successful" : "failed") << std::endl << std::endl;

	// print program after modifications
	std::cout << "Transformed program: " << std::endl;
	program.name += " (transformed)";
	cola::print(program, std::cout);

	// assertion check
	std::cout << std::endl << "Checking assertions... " << std::flush;
	bool assertions_safe = discharge_assertions(program, table);
	std::cout << "done" << std::endl;
	std::cout << "Assertion check: " << (assertions_safe ? "successful" : "failed") << std::endl;
	std::cout << std::endl;

	// throw std::logic_error("breakpoint");
	std::cout << "Checking linearizability under GC... " << std::flush;
	bool linearizable = prtypes::check_linearizability(program);
	std::cout << "done" << std::endl;
	std::cout << "Linearizability check: " << (linearizable ? "successful" : "failed") << std::endl;

	std::cout << std::endl;
	bool verification_successful = type_safe && assertions_safe && linearizable;
	std::cout << "    #############################" << std::endl;
	std::cout << "    ##  Summary                ##" << std::endl;
	std::cout << "    ##  -------                ##" << std::endl;
	std::cout << "    ##                         ##" << std::endl;
	std::cout << "    ##  Type safe:        " << (type_safe ? "yes" : " no") << "  ##" << std::endl;
	std::cout << "    ##  Assertions hold:  " << (assertions_safe ? "yes" : " no") << "  ##" << std::endl;
	std::cout << "    ##  Linearizable:     " << (linearizable ? "yes" : " no") << "  ##" << std::endl;
	std::cout << "    ##                         ##" << std::endl;
	std::cout << "    ##   ==> VERIFICATION      ##" << std::endl;
	std::cout << "    ##       " << (verification_successful ? " SUCCESSFUL " : "   FAILED   ") << "      ##" << std::endl;
	std::cout << "    #############################" << std::endl << std::endl;

	if (type_safe && assertions_safe && linearizable) {
		return 0;
	} else {
		return 1;
	}
}
