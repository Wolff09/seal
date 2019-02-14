#include "types/factory.hpp"

using namespace cola;
using namespace prtypes;


inline std::pair<const Function&, const Type&> get_basics() {
	static const Function& free_function = Observer::free_function();
	static const Type& ptrtype = free_function.args.at(0)->type;
	return { free_function, ptrtype };
}

inline bool takes_single_pointer(const Function& function) {
	return function.args.size() == 1 && function.args.at(0)->type.sort == Sort::PTR;
}

inline std::unique_ptr<Transition> mk_transition_invocation_any_thread(const State& src, const State& dst, const Function& function, const ObserverVariable& obsvar) {
	auto guard = std::make_unique<EqGuard>(obsvar, std::make_unique<ArgumentGuardVariable>(*function.args.at(0)));
	return std::make_unique<Transition>(src, dst, function, Transition::INVOCATION, std::move(guard));
}

inline std::unique_ptr<Transition> mk_transition_invocation_self(const State& src, const State& dst, const Function& func, const ObserverVariable& obsvar_thread, const ObserverVariable& obsvar_arg) {
	auto guard_thr = std::make_unique<EqGuard>(obsvar_thread, std::make_unique<SelfGuardVariable>());
	auto guard_arg = std::make_unique<EqGuard>(obsvar_arg, std::make_unique<ArgumentGuardVariable>(*func.args.at(0)));
	auto guard = std::make_unique<ConjunctionGuard>(std::move(guard_thr), std::move(guard_arg));
	return std::make_unique<Transition>(src, dst, func, Transition::INVOCATION, std::move(guard));
}

inline std::unique_ptr<Transition> mk_transition_invocation_self_neg(const State& src, const State& dst, const Function& func, const ObserverVariable& obsvar_thread, const ObserverVariable& obsvar_arg) {
	auto guard_thr = std::make_unique<EqGuard>(obsvar_thread, std::make_unique<SelfGuardVariable>());
	auto guard_arg = std::make_unique<NeqGuard>(obsvar_arg, std::make_unique<ArgumentGuardVariable>(*func.args.at(0)));
	auto guard = std::make_unique<ConjunctionGuard>(std::move(guard_thr), std::move(guard_arg));
	return std::make_unique<Transition>(src, dst, func, Transition::INVOCATION, std::move(guard));
}

inline std::unique_ptr<Transition> mk_transition_response_self(const State& src, const State& dst, const Function& func, const ObserverVariable& obsvar_thread) {
	auto guard = std::make_unique<EqGuard>(obsvar_thread, std::make_unique<SelfGuardVariable>());
	return std::make_unique<Transition>(src, dst, func, Transition::RESPONSE, std::move(guard));
}


std::unique_ptr<Observer> prtypes::make_active_local_guarantee_observer(const Function& retire_function, std::string name_prefix) {
	auto [free_function, ptrtype] = get_basics();
	assert(retire_function.args.size() == 1);
	assert(retire_function.args.at(0)->type.sort == Sort::PTR);

	auto result = std::make_unique<Observer>();
	result->negative_specification = false;
	
	// variables
	result->variables.push_back(std::make_unique<ThreadObserverVariable>(name_prefix + ":thread"));
	result->variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>(name_prefix + ":address", ptrtype, false)));

	// states
	result->states.push_back(std::make_unique<State>(name_prefix + ".0", true, true));
	result->states.push_back(std::make_unique<State>(name_prefix + ".1", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".2", false, false));

	// transitions
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(0), *result->states.at(1), retire_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(0), *result->states.at(2), free_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(1), *result->states.at(0), free_function, *result->variables.at(1)));

	return result;
}

std::unique_ptr<Observer> prtypes::make_base_smr_observer(const Function& retire_function) {
	auto [free_function, ptrtype] = get_basics();
	assert(retire_function.args.size() == 1);
	assert(retire_function.args.at(0)->type.sort == Sort::PTR);
	std::string name_prefix = "SmrBase";

	auto result = std::make_unique<Observer>();
	result->negative_specification = true;
	
	// variables
	result->variables.push_back(std::make_unique<ThreadObserverVariable>(name_prefix + ":thread"));
	result->variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>(name_prefix + ":address", ptrtype, false)));

	// states
	result->states.push_back(std::make_unique<State>(name_prefix + ".0", true, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".1", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".2", false, true));

	// transitions
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(0), *result->states.at(1), retire_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(0), *result->states.at(2), free_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(1), *result->states.at(0), free_function, *result->variables.at(1)));

	return result;
}

std::unique_ptr<Observer> prtypes::make_hp_no_transfer_observer(const Function& retire_function, const Function& protect_function, std::string id) {
	auto [free_function, ptrtype] = get_basics();
	assert(retire_function.args.size() == 1);
	assert(retire_function.args.at(0)->type.sort == Sort::PTR);
	std::string name_prefix = "HP" + id;

	auto result = std::make_unique<Observer>();
	result->negative_specification = true;

	// variables
	result->variables.push_back(std::make_unique<ThreadObserverVariable>(name_prefix + ":thread"));
	result->variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>(name_prefix + ":address", ptrtype, false)));

	// states
	result->states.push_back(std::make_unique<State>(name_prefix + ".0", true, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".1", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".2", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".3", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".4", false, true));

	// 'forward' transitions
	result->transitions.push_back(mk_transition_invocation_self(*result->states.at(0), *result->states.at(1), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_response_self(*result->states.at(1), *result->states.at(2), protect_function, *result->variables.at(0)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(2), *result->states.at(3), retire_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(3), *result->states.at(4), free_function, *result->variables.at(1)));

	// 'backward' transitions
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(1), *result->states.at(0), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(2), *result->states.at(0), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(3), *result->states.at(0), protect_function, *result->variables.at(0), *result->variables.at(1)));

	return result;
}

std::vector<std::unique_ptr<cola::Observer>> prtypes::make_hp_not_transfer_guarantee_observers(const cola::Function& /*retire_function*/, const cola::Function& /*protect_function*/, std::string /*id*/) {
	throw std::logic_error("not yet implemented: prtypes::make_hp_not_transfer_guarantee_observers");
}
