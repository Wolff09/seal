#include "types/check.hpp"
#include "types/checker.hpp"
#include "types/inference.hpp"
#include "types/observer.hpp"
#include "types/factory.hpp"
#include "types/guarantees.hpp"
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "cola/parse.hpp"
#include "cola/util.hpp"

using namespace cola;
using namespace prtypes;


bool prtypes::type_check(const Program& program, const GuaranteeTable& guarantee_table) {
	TypeChecker checker(program, guarantee_table);
	return checker.is_well_typed();
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void print_guarantees(const GuaranteeSet set) {
	std::cout << "{";
	for (const auto& g : set) {
		std::cout << g.get().name << /*"[" << &(g.get()) << "]" <<*/ ", ";
	}
	std::cout << "}" << std::endl;
}

std::optional<std::reference_wrapper<const Function>> find_function(const Program& program, std::string name) {
	for (const auto& function : program.functions) {
		if (function->name == name) {
			return *function;
		}
	}
	return std::nullopt;
}

void prtypes::test() {
	// throw std::logic_error("not implemented");
	
	std::cout << std::endl << std::endl << "Testing..." << std::endl;
	// const Type& ptrtype = Observer::free_function().args.at(0)->type;

	std::string filename = "/Users/wolff/Desktop/PointerRaceTypes/test.cola";
	auto program_ptr = cola::parse(filename);
	Program& program = *program_ptr;
	program.name = "TestProgram";
	
	std::cout << "Parsed program: " << std::endl;
	cola::print(program, std::cout);

	// query retire
	auto search_retire = find_function(program, "retire");
	auto search_protect1 = find_function(program, "protect1");
	auto search_protect2 = find_function(program, "protect2");
	assert(search_protect1.has_value());
	assert(search_protect2.has_value());
	const Function& retire = *search_retire;
	const Function& protect1 = *search_protect1;
	const Function& protect2 = *search_protect2;

	// create stuff
	std::cout << std::endl << "Computing simulation... " << std::flush;
	SmrObserverStore store(program, retire);
	store.add_impl_observer(make_hp_no_transfer_observer(retire, protect1));
	GuaranteeTable table(store);
	std::cout << "done" << std::endl;

	// adding guarantees
	auto add_guarantees = [&](const Function& func) {
		std::cout << std::endl << "Adding guarantees for " << func.name << "... " << std::flush;
		auto hp_guarantee_observers = prtypes::make_hp_no_transfer_guarantee_observers(retire, protect1, func.name);
		auto& guarantee_e1 = table.add_guarantee(std::move(hp_guarantee_observers.at(0)), "E1-" + func.name);
		auto& guarantee_e2 = table.add_guarantee(std::move(hp_guarantee_observers.at(1)), "E2-" + func.name);
		auto& guarantee_p = table.add_guarantee(std::move(hp_guarantee_observers.at(2)), "P-" + func.name);
		std::cout << "done" << std::endl;
		std::cout << "  - " << guarantee_e1.name << ": (transient, valid) = (" << guarantee_e1.is_transient << ", " << guarantee_e1.entails_validity << ")" << std::endl;
		std::cout << "  - " << guarantee_e2.name << ": (transient, valid) = (" << guarantee_e2.is_transient << ", " << guarantee_e2.entails_validity << ")" << std::endl;
		std::cout << "  -  " << guarantee_p.name << ": (transient, valid) = (" << guarantee_p.is_transient << ", " << guarantee_p.entails_validity << ")" << std::endl;
	};
	add_guarantees(protect1);
	add_guarantees(protect2);

	// type check
	std::cout << std::endl << "Checking typing..." << std::endl;
	bool type_safe = type_check(program, table);
	std::cout << "Type check: " << (type_safe ? "successful" : "failed") << std::endl;

	// // safe predicate
	// bool safe_valid, safe_invalid;
	// std::cout << std::endl << "Checking safe predicate. " << std::endl;
	// Enter enter_retire(retire);
	// VariableDeclaration ptr("ptr", ptrtype, false);
	// enter_retire.args.push_back(std::make_unique<VariableExpression>(ptr));
	// safe_valid = store.simulation.is_safe(enter_retire, { ptr }, {  });
	// std::cout << "  - enter retire(<valid>):   " << (safe_valid ? "safe" : "not safe") << std::endl;
	// safe_invalid = store.simulation.is_safe(enter_retire, { ptr }, { ptr });
	// std::cout << "  - enter retire(<invalid>): " << (safe_invalid ? "safe" : "not safe") << std::endl;

	// Enter enter_protect(protect1);
	// enter_protect.args.push_back(std::make_unique<VariableExpression>(ptr));
	// safe_valid = store.simulation.is_safe(enter_protect, { ptr }, {  });
	// std::cout << "  - enter protect(<valid>):   " << (safe_valid ? "safe" : "not safe") << std::endl;
	// safe_invalid = store.simulation.is_safe(enter_protect, { ptr }, { ptr });
	// std::cout << "  - enter protect(<invalid>): " << (safe_invalid ? "safe" : "not safe") << std::endl;




	// std::cout << std::endl << std::endl << "Testing..." << std::endl;
	// const Type& ptrtype = Observer::free_function().args.at(0)->type;

	// Program program;
	// program.name = "TestProgram";
	// program.functions.push_back(std::make_unique<Function>("retire", Type::void_type(), Function::SMR));
	// Function& retire = *program.functions.at(0);
	// retire.args.push_back(std::make_unique<VariableDeclaration>("ptr", ptrtype, false));

	// // active observer (simplified)
	// auto obs_active_ptr = std::make_unique<Observer>();
	// auto& obs_active = *obs_active_ptr;
	// obs_active.negative_specification = false;
	// obs_active.variables.push_back(std::make_unique<ThreadObserverVariable>("T"));
	// obs_active.variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>("P", ptrtype, false)));

	// obs_active.states.push_back(std::make_unique<State>("A.active", true, true));
	// obs_active.states.push_back(std::make_unique<State>("A.retired", false, false));
	// obs_active.states.push_back(std::make_unique<State>("A.dfreed", false, false));

	// obs_active.transitions.push_back(mk_transition(*obs_active.states.at(0), *obs_active.states.at(1), retire, *obs_active.variables.at(1)));
	// obs_active.transitions.push_back(mk_transition(*obs_active.states.at(1), *obs_active.states.at(0), Observer::free_function(), *obs_active.variables.at(1)));
	// obs_active.transitions.push_back(mk_transition(*obs_active.states.at(0), *obs_active.states.at(2), Observer::free_function(), *obs_active.variables.at(1)));

	// // safe observer (simplified)
	// auto obs_safe_ptr = std::make_unique<Observer>();
	// auto& obs_safe = *obs_safe_ptr;
	// obs_safe.negative_specification = false;
	// obs_safe.variables.push_back(std::make_unique<ThreadObserverVariable>("T"));
	// obs_safe.variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>("P", ptrtype, false)));

	// obs_safe.states.push_back(std::make_unique<State>("S.init", true, true));
	// obs_safe.states.push_back(std::make_unique<State>("S.retired", true, true));
	// obs_safe.states.push_back(std::make_unique<State>("S.freed", false, false));

	// obs_safe.transitions.push_back(mk_transition(*obs_safe.states.at(0), *obs_safe.states.at(1), retire, *obs_safe.variables.at(1)));
	// obs_safe.transitions.push_back(mk_transition(*obs_safe.states.at(1), *obs_safe.states.at(2), Observer::free_function(), *obs_safe.variables.at(1)));


	// // guarantees
	// Guarantee gactive("active", std::move(obs_active_ptr));
	// Guarantee gsafe("safe", std::move(obs_safe_ptr));

	// GuaranteeSet gall = { gactive, gsafe };
	// std::cout << "guarantees: ";
	// print_guarantees(gall);

	// // inference
	// InferenceEngine ie(program, gall);

	// // -> epsilon
	// std::cout << "infer epsilon: ";
	// auto inferEpsilon = ie.infer(gall);
	// print_guarantees(inferEpsilon);
	// assert(inferEpsilon.count(gactive) && inferEpsilon.count(gsafe));

	// // -> enter
	// Enter enter(retire);
	// VariableDeclaration ptr("ptr", ptrtype, false);
	// enter.args.push_back(std::make_unique<VariableExpression>(ptr));
	// std::cout << "infer enter:retire(ptr): " << std::flush;
	// auto inferEnter = ie.infer_enter(gall, enter, ptr);
	// print_guarantees(inferEnter);
	// assert(inferEnter.count(gactive) && inferEnter.count(gsafe));
}
