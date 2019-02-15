#include "types/check.hpp"
#include "types/checker.hpp"
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "types/inference.hpp"
#include "types/observer.hpp"
#include "types/factory.hpp"
#include "types/guarantees.hpp"

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

std::unique_ptr<Transition> mk_transition_any_thread(const State& src, const State& dst, const Function& func, const ObserverVariable& obsvar) {
	auto guard = std::make_unique<EqGuard>(obsvar, std::make_unique<ArgumentGuardVariable>(*func.args.at(0)));
	return std::make_unique<Transition>(src, dst, func, Transition::INVOCATION, std::move(guard));
}

std::unique_ptr<Transition> mk_transition_invocation_self(const State& src, const State& dst, const Function& func, const ObserverVariable& obsvar_thread, const ObserverVariable& obsvar_arg) {
	auto guard_thr = std::make_unique<EqGuard>(obsvar_thread, std::make_unique<SelfGuardVariable>());
	auto guard_arg = std::make_unique<EqGuard>(obsvar_arg, std::make_unique<ArgumentGuardVariable>(*func.args.at(0)));
	auto guard = std::make_unique<ConjunctionGuard>(std::move(guard_thr), std::move(guard_arg));
	return std::make_unique<Transition>(src, dst, func, Transition::INVOCATION, std::move(guard));
}

std::unique_ptr<Transition> mk_transition_invocation_self_neg(const State& src, const State& dst, const Function& func, const ObserverVariable& obsvar_thread, const ObserverVariable& obsvar_arg) {
	auto guard_thr = std::make_unique<EqGuard>(obsvar_thread, std::make_unique<SelfGuardVariable>());
	auto guard_arg = std::make_unique<NeqGuard>(obsvar_arg, std::make_unique<ArgumentGuardVariable>(*func.args.at(0)));
	auto guard = std::make_unique<ConjunctionGuard>(std::move(guard_thr), std::move(guard_arg));
	return std::make_unique<Transition>(src, dst, func, Transition::INVOCATION, std::move(guard));
}

std::unique_ptr<Transition> mk_transition_response_self(const State& src, const State& dst, const Function& func, const ObserverVariable& obsvar_thread) {
	auto guard = std::make_unique<EqGuard>(obsvar_thread, std::make_unique<SelfGuardVariable>());
	return std::make_unique<Transition>(src, dst, func, Transition::RESPONSE, std::move(guard));
}

void prtypes::test() {
	// throw std::logic_error("not implemented");
	
	std::cout << std::endl << std::endl << "Testing..." << std::endl;
	const Type& ptrtype = Observer::free_function().args.at(0)->type;

	Program program;
	program.name = "TestProgram";

	// create retire function
	program.functions.push_back(std::make_unique<Function>(" retire", Type::void_type(), Function::SMR));
	Function& retire = *program.functions.back();
	retire.args.push_back(std::make_unique<VariableDeclaration>("ptr", ptrtype, false));

	// create protect function
	program.functions.push_back(std::make_unique<Function>("protect", Type::void_type(), Function::SMR));
	Function& protect = *program.functions.back();
	protect.args.push_back(std::make_unique<VariableDeclaration>("ptr", ptrtype, false));

	// create stuff
	std::cout << std::endl << "Computing simulation... " << std::flush;
	SmrObserverStore store(program, retire);
	store.add_impl_observer(make_hp_no_transfer_observer(retire, protect));
	std::cout << "done" << std::endl;

	std::cout << "Adding guarantees... " << std::flush;
	GuaranteeTable table(store);
	auto hp_guarantee_observers = prtypes::make_hp_no_transfer_guarantee_observers(retire, protect, "protect");
	auto& guarantee_e1 = table.add_guarantee(std::move(hp_guarantee_observers.at(0)), "E1");
	auto& guarantee_e2 = table.add_guarantee(std::move(hp_guarantee_observers.at(1)), "E2");
	auto& guarantee_p = table.add_guarantee(std::move(hp_guarantee_observers.at(2)), "P");
	std::cout << "done" << std::endl;
	std::cout << "  - E1: (transient, valid) = (" << guarantee_e1.is_transient << ", " << guarantee_e1.entails_validity << ")" << std::endl;
	std::cout << "  - E2: (transient, valid) = (" << guarantee_e2.is_transient << ", " << guarantee_e2.entails_validity << ")" << std::endl;
	std::cout << "  -  P: (transient, valid) = (" << guarantee_p.is_transient << ", " << guarantee_p.entails_validity << ")" << std::endl;

	// safe predicate
	bool safe_valid, safe_invalid;
	std::cout << std::endl << "Checking safe predicate. " << std::endl;
	Enter enter_retire(retire);
	VariableDeclaration ptr("ptr", ptrtype, false);
	enter_retire.args.push_back(std::make_unique<VariableExpression>(ptr));
	safe_valid = store.simulation.is_safe(enter_retire, { ptr }, {  });
	std::cout << "  - enter retire(<valid>):   " << (safe_valid ? "safe" : "not safe") << std::endl;
	safe_invalid = store.simulation.is_safe(enter_retire, { ptr }, { ptr });
	std::cout << "  - enter retire(<invalid>): " << (safe_invalid ? "safe" : "not safe") << std::endl;

	Enter enter_protect(protect);
	enter_protect.args.push_back(std::make_unique<VariableExpression>(ptr));
	safe_valid = store.simulation.is_safe(enter_protect, { ptr }, {  });
	std::cout << "  - enter protect(<valid>):   " << (safe_valid ? "safe" : "not safe") << std::endl;
	safe_invalid = store.simulation.is_safe(enter_protect, { ptr }, { ptr });
	std::cout << "  - enter protect(<invalid>): " << (safe_invalid ? "safe" : "not safe") << std::endl;




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
