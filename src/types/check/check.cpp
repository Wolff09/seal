#include "types/check.hpp"
#include "types/check/check_impl.hpp"
#include "types/inference.hpp"

using namespace cola;
using namespace prtypes;


bool prtypes::type_check(const cola::Program& /*program*/, const GuaranteeTable& /*guarantee_table*/) {
	throw std::logic_error("not yet implemented (prtypes::type_check)");
}




////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
#include "cola/ast.hpp"
#include "cola/observer.hpp"

using namespace cola;
using namespace prtypes;

void print_guarantees(const GuaranteeSet set) {
	std::cout << "{";
	for (const auto& g : set) {
		std::cout << g.get().name << /*"[" << &(g.get()) << "]" <<*/ ", ";
	}
	std::cout << "}" << std::endl;
}

std::unique_ptr<Transition> mk_transition(const State& src, const State& dst, const Function& func, const ObserverVariable& obsvar) {
	auto guard = std::make_unique<EqGuard>(obsvar, std::make_unique<ArgumentGuardVariable>(*func.args.at(0)));
	return std::make_unique<Transition>(src, dst, func, Transition::INVOCATION, std::move(guard));
}

void prtypes::test() {
	std::cout << std::endl << std::endl << "Testing..." << std::endl;
	const Type& ptrtype = Observer::free_function().args.at(0)->type;

	Program program;
	program.name = "TestProgram";
	program.functions.push_back(std::make_unique<Function>("retire", Type::void_type(), Function::SMR));
	Function& retire = *program.functions.at(0);
	retire.args.push_back(std::make_unique<VariableDeclaration>("ptr", ptrtype, false));

	// active observer (simplified)
	auto obs_active_ptr = std::make_unique<Observer>();
	auto& obs_active = *obs_active_ptr;
	obs_active.negative_specification = false;
	obs_active.variables.push_back(std::make_unique<ThreadObserverVariable>("T"));
	obs_active.variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>("P", ptrtype, false)));

	obs_active.states.push_back(std::make_unique<State>("A.active", true, true));
	obs_active.states.push_back(std::make_unique<State>("A.retired", false, false));
	obs_active.states.push_back(std::make_unique<State>("A.dfreed", false, false));

	obs_active.transitions.push_back(mk_transition(*obs_active.states.at(0), *obs_active.states.at(1), retire, *obs_active.variables.at(1)));
	obs_active.transitions.push_back(mk_transition(*obs_active.states.at(1), *obs_active.states.at(0), Observer::free_function(), *obs_active.variables.at(1)));
	obs_active.transitions.push_back(mk_transition(*obs_active.states.at(0), *obs_active.states.at(2), Observer::free_function(), *obs_active.variables.at(1)));

	// safe observer (simplified)
	auto obs_safe_ptr = std::make_unique<Observer>();
	auto& obs_safe = *obs_safe_ptr;
	obs_safe.negative_specification = false;
	obs_safe.variables.push_back(std::make_unique<ThreadObserverVariable>("T"));
	obs_safe.variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>("P", ptrtype, false)));

	obs_safe.states.push_back(std::make_unique<State>("S.init", true, true));
	obs_safe.states.push_back(std::make_unique<State>("S.retired", true, true));
	obs_safe.states.push_back(std::make_unique<State>("S.freed", false, false));

	obs_safe.transitions.push_back(mk_transition(*obs_safe.states.at(0), *obs_safe.states.at(1), retire, *obs_safe.variables.at(1)));
	obs_safe.transitions.push_back(mk_transition(*obs_safe.states.at(1), *obs_safe.states.at(2), Observer::free_function(), *obs_safe.variables.at(1)));


	// guarantees
	Guarantee gactive("active", std::move(obs_active_ptr));
	Guarantee gsafe("safe", std::move(obs_safe_ptr));

	GuaranteeSet gall = { gactive, gsafe };
	std::cout << "guarantees: ";
	print_guarantees(gall);

	// inference
	InferenceEngine ie(program, gall);

	// -> epsilon
	std::cout << "infer epsilon: ";
	auto inferEpsilon = ie.infer(gall);
	print_guarantees(inferEpsilon);
	assert(inferEpsilon.count(gactive) && inferEpsilon.count(gsafe));

	// -> enter
	Enter enter(retire);
	VariableDeclaration ptr("ptr", ptrtype, false);
	enter.args.push_back(std::make_unique<VariableExpression>(ptr));
	std::cout << "infer enter:retire(ptr): " << std::flush;
	auto inferEnter = ie.infer_enter(gall, enter, ptr);
	print_guarantees(inferEnter);
	assert(inferEnter.count(gactive) && inferEnter.count(gsafe));
}
