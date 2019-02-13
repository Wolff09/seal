#include "types/check.hpp"
#include "types/checker.hpp"
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "types/inference.hpp"

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

	// hp observer
	auto obs_hp_ptr = std::make_unique<Observer>();
	auto& obs_hp = *obs_hp_ptr;
	obs_hp.negative_specification = true;
	obs_hp.variables.push_back(std::make_unique<ThreadObserverVariable>("T"));
	obs_hp.variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>("P", ptrtype, false)));

	obs_hp.states.push_back(std::make_unique<State>("HP.0", true, false));
	obs_hp.states.push_back(std::make_unique<State>("HP.1", false, false));
	obs_hp.states.push_back(std::make_unique<State>("HP.2", false, false));
	obs_hp.states.push_back(std::make_unique<State>("HP.3", false, false));
	obs_hp.states.push_back(std::make_unique<State>("HP.4", false, true));

	obs_hp.transitions.push_back(mk_transition_invocation_self(*obs_hp.states.at(0), *obs_hp.states.at(1), protect, *obs_hp.variables.at(0), *obs_hp.variables.at(1)));
	obs_hp.transitions.push_back(mk_transition_response_self(*obs_hp.states.at(1), *obs_hp.states.at(2), protect, *obs_hp.variables.at(0)));
	obs_hp.transitions.push_back(mk_transition_any_thread(*obs_hp.states.at(2), *obs_hp.states.at(3), retire, *obs_hp.variables.at(1)));
	obs_hp.transitions.push_back(mk_transition_any_thread(*obs_hp.states.at(3), *obs_hp.states.at(4), Observer::free_function(), *obs_hp.variables.at(1)));

	obs_hp.transitions.push_back(mk_transition_invocation_self_neg(*obs_hp.states.at(1), *obs_hp.states.at(0), protect, *obs_hp.variables.at(0), *obs_hp.variables.at(1)));
	obs_hp.transitions.push_back(mk_transition_invocation_self_neg(*obs_hp.states.at(2), *obs_hp.states.at(0), protect, *obs_hp.variables.at(0), *obs_hp.variables.at(1)));
	obs_hp.transitions.push_back(mk_transition_invocation_self_neg(*obs_hp.states.at(3), *obs_hp.states.at(0), protect, *obs_hp.variables.at(0), *obs_hp.variables.at(1)));

	std::cout << std::endl << "Computing simulation... " << std::flush;
	SimulationEngine engine;
	engine.compute_simulation(obs_hp);
	std::cout << "done" << std::endl;


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
