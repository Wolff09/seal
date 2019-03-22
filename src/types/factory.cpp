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
	assert(takes_single_pointer(free_function));
	assert(takes_single_pointer(retire_function));

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
	assert(takes_single_pointer(free_function));
	assert(takes_single_pointer(retire_function));
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
	assert(takes_single_pointer(free_function));
	assert(takes_single_pointer(retire_function));
	assert(takes_single_pointer(protect_function));
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

std::unique_ptr<cola::Observer> prtypes::make_hp_transfer_observer(const cola::Function& retire_function, const cola::Function& protect_function, const cola::Function& transfer_protect_function, std::string id) {
	auto [free_function, ptrtype] = get_basics();
	assert(takes_single_pointer(free_function));
	assert(takes_single_pointer(retire_function));
	assert(takes_single_pointer(protect_function));
	assert(takes_single_pointer(transfer_protect_function));
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
	result->states.push_back(std::make_unique<State>(name_prefix + ".5", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".6", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".7", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".8", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".9", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".10", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".11", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".12", false, true));
	result->states.push_back(std::make_unique<State>(name_prefix + ".13", false, true));

	// protect observed
	result->transitions.push_back(mk_transition_invocation_self(*result->states.at(0), *result->states.at(1), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self(*result->states.at(6), *result->states.at(8), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self(*result->states.at(7), *result->states.at(9), protect_function, *result->variables.at(0), *result->variables.at(1)));

	// protect exit
	result->transitions.push_back(mk_transition_response_self(*result->states.at(1), *result->states.at(2), protect_function, *result->variables.at(0)));
	result->transitions.push_back(mk_transition_response_self(*result->states.at(8), *result->states.at(10), protect_function, *result->variables.at(0)));
	result->transitions.push_back(mk_transition_response_self(*result->states.at(9), *result->states.at(11), protect_function, *result->variables.at(0)));

	// protect unobserved
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(1), *result->states.at(0), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(2), *result->states.at(0), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(3), *result->states.at(0), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(12), *result->states.at(0), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(13), *result->states.at(0), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(10), *result->states.at(6), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(11), *result->states.at(7), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(8), *result->states.at(6), protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(9), *result->states.at(7), protect_function, *result->variables.at(0), *result->variables.at(1)));

	// transfer observed
	result->transitions.push_back(mk_transition_invocation_self(*result->states.at(0), *result->states.at(5), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self(*result->states.at(2), *result->states.at(12), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self(*result->states.at(3), *result->states.at(13), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));

	// transfer exit
	result->transitions.push_back(mk_transition_response_self(*result->states.at(5), *result->states.at(6), transfer_protect_function, *result->variables.at(0)));
	result->transitions.push_back(mk_transition_response_self(*result->states.at(12), *result->states.at(10), transfer_protect_function, *result->variables.at(0)));
	result->transitions.push_back(mk_transition_response_self(*result->states.at(13), *result->states.at(11), transfer_protect_function, *result->variables.at(0)));

	// transfer unobserved
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(5), *result->states.at(0), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(6), *result->states.at(0), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(7), *result->states.at(0), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(8), *result->states.at(0), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(9), *result->states.at(0), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(10), *result->states.at(2), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(12), *result->states.at(2), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(11), *result->states.at(3), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_self_neg(*result->states.at(13), *result->states.at(3), transfer_protect_function, *result->variables.at(0), *result->variables.at(1)));

	// retire
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(2), *result->states.at(3), retire_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(12), *result->states.at(13), retire_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(10), *result->states.at(11), retire_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(8), *result->states.at(9), retire_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(6), *result->states.at(7), retire_function, *result->variables.at(1)));

	// free
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(3), *result->states.at(4), free_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(13), *result->states.at(4), free_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(11), *result->states.at(4), free_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(9), *result->states.at(4), free_function, *result->variables.at(1)));
	result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(7), *result->states.at(4), free_function, *result->variables.at(1)));

	return result;
}

std::vector<std::unique_ptr<Observer>> prtypes::make_hp_no_transfer_guarantee_observers(const Function& retire_function, const Function& protect_function, std::string id) {
	auto [free_function, ptrtype] = get_basics();
	assert(takes_single_pointer(free_function));
	assert(takes_single_pointer(retire_function));
	assert(takes_single_pointer(protect_function));

	// observer guarantee: enter protect has been called, not yet exited
	auto obs_entered = prtypes::make_hp_no_transfer_observer(retire_function, protect_function, "-E1" + id);
	obs_entered->negative_specification = false;
	obs_entered->states.at(0)->final = false;
	obs_entered->states.at(1)->final = true;
	obs_entered->states.at(2)->final = true;
	obs_entered->states.at(3)->final = true;
	obs_entered->states.at(4)->final = true;

	// observer guarantee: enter protect has been called and exited
	auto obs_exited = prtypes::make_hp_no_transfer_observer(retire_function, protect_function, "-E2" + id);
	obs_exited->negative_specification = false;
	obs_exited->states.at(0)->final = false;
	obs_exited->states.at(1)->final = false;
	obs_exited->states.at(2)->final = true;
	obs_exited->states.at(3)->final = true;
	obs_exited->states.at(4)->final = true;

	// observer guarantee: enter protect has been called and exited and the protection is sucessful
	auto obs_safe = std::make_unique<Observer>();
	obs_safe->negative_specification = false;

	// variables
	obs_safe->variables.push_back(std::make_unique<ThreadObserverVariable>("HP-P:thread"));
	obs_safe->variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>("HP-P:address", ptrtype, false)));

	// states
	obs_safe->states.push_back(std::make_unique<State>("HP-P.0", true, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-P.1", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-P.2", false, true));
	obs_safe->states.push_back(std::make_unique<State>("HP-P.3", false, true));
	obs_safe->states.push_back(std::make_unique<State>("HP-P.4", false, true));
	obs_safe->states.push_back(std::make_unique<State>("HP-P.5", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-P.6", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-P.7", false, false)); // TODO: should this state be final?
	obs_safe->states.push_back(std::make_unique<State>("HP-P.8", false, false));

	// 'forward' transitions
	obs_safe->transitions.push_back(mk_transition_invocation_self(*obs_safe->states.at(0), *obs_safe->states.at(1), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_response_self(*obs_safe->states.at(1), *obs_safe->states.at(2), protect_function, *obs_safe->variables.at(0)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(2), *obs_safe->states.at(3), retire_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(3), *obs_safe->states.at(4), free_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self(*obs_safe->states.at(5), *obs_safe->states.at(6), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_response_self(*obs_safe->states.at(6), *obs_safe->states.at(7), protect_function, *obs_safe->variables.at(0)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(7), *obs_safe->states.at(3), retire_function, *obs_safe->variables.at(1)));

	// 'backward' transitions
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(1), *obs_safe->states.at(0), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(2), *obs_safe->states.at(0), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(3), *obs_safe->states.at(5), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(6), *obs_safe->states.at(5), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(7), *obs_safe->states.at(5), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));

	// 'to copy'
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(0), *obs_safe->states.at(5), retire_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(1), *obs_safe->states.at(6), retire_function, *obs_safe->variables.at(1)));

	// 'from copy'
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(5), *obs_safe->states.at(0), free_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(6), *obs_safe->states.at(1), free_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(7), *obs_safe->states.at(2), free_function, *obs_safe->variables.at(1)));

	// additional transitions: double free
	// obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(0), *obs_safe->states.at(8), free_function, *obs_safe->variables.at(1)));
	// obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(1), *obs_safe->states.at(8), free_function, *obs_safe->variables.at(1)));
	// obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(2), *obs_safe->states.at(4), free_function, *obs_safe->variables.at(1)));

	std::vector<std::unique_ptr<Observer>> result;
	result.push_back(std::move(obs_entered));
	result.push_back(std::move(obs_exited));
	result.push_back(std::move(obs_safe));
	return result;
}

std::vector<std::unique_ptr<cola::Observer>> prtypes::make_hp_transfer_guarantee_observers(const cola::Function& retire_function, const cola::Function& protect_function, const cola::Function& transfer_protect_function, std::string id) {
	auto [free_function, ptrtype] = get_basics();
	assert(takes_single_pointer(free_function));
	assert(takes_single_pointer(retire_function));
	assert(takes_single_pointer(protect_function));

	std::vector<std::unique_ptr<cola::Observer>> result = make_hp_no_transfer_guarantee_observers(retire_function, protect_function, id);

	auto obs_prep = std::make_unique<Observer>();
	obs_prep->negative_specification = false;

	// variables
	obs_prep->variables.push_back(std::make_unique<ThreadObserverVariable>("HP-E3:thread"));
	obs_prep->variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>("HP-E3:address", ptrtype, false)));

	// states
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.0", true, false));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.1", false, false));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.2", false, false));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.3", false, false));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.4", false, true));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.5", false, false));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.6", false, false));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.7", false, false));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.8", false, true));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.9", false, true));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.10", false, true));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.11", false, true));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.12", false, false));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.13", false, true));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.14", false, true));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.15", false, true));
	obs_prep->states.push_back(std::make_unique<State>("HP-E3.16", false, true));

	// transitions for transfer_protect_function protection
	obs_prep->transitions.push_back(mk_transition_invocation_self(*obs_prep->states.at(0), *obs_prep->states.at(1), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_response_self(*obs_prep->states.at(1), *obs_prep->states.at(2), transfer_protect_function, *obs_prep->variables.at(0)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(2), *obs_prep->states.at(3), retire_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(3), *obs_prep->states.at(4), free_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self(*obs_prep->states.at(5), *obs_prep->states.at(6), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_response_self(*obs_prep->states.at(6), *obs_prep->states.at(7), transfer_protect_function, *obs_prep->variables.at(0)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(7), *obs_prep->states.at(3), retire_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(1), *obs_prep->states.at(0), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(2), *obs_prep->states.at(0), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(3), *obs_prep->states.at(5), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(6), *obs_prep->states.at(5), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(7), *obs_prep->states.at(5), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(0), *obs_prep->states.at(5), retire_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(1), *obs_prep->states.at(6), retire_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(5), *obs_prep->states.at(0), free_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(6), *obs_prep->states.at(1), free_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(7), *obs_prep->states.at(2), free_function, *obs_prep->variables.at(1)));

	// transitions for tranfering protection
	obs_prep->transitions.push_back(mk_transition_invocation_self(*obs_prep->states.at(2), *obs_prep->states.at(8), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self(*obs_prep->states.at(3), *obs_prep->states.at(9), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_response_self(*obs_prep->states.at(8), *obs_prep->states.at(10), protect_function, *obs_prep->variables.at(0)));
	obs_prep->transitions.push_back(mk_transition_response_self(*obs_prep->states.at(9), *obs_prep->states.at(11), protect_function, *obs_prep->variables.at(0)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(8), *obs_prep->states.at(9), retire_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(10), *obs_prep->states.at(11), retire_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(9), *obs_prep->states.at(4), free_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(11), *obs_prep->states.at(4), free_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(8), *obs_prep->states.at(0), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(9), *obs_prep->states.at(5), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(8), *obs_prep->states.at(2), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(10), *obs_prep->states.at(2), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(9), *obs_prep->states.at(3), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(11), *obs_prep->states.at(3), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));

	obs_prep->transitions.push_back(mk_transition_invocation_self(*obs_prep->states.at(7), *obs_prep->states.at(12), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(12), *obs_prep->states.at(9), retire_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(12), *obs_prep->states.at(5), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(12), *obs_prep->states.at(7), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(12), *obs_prep->states.at(8), free_function, *obs_prep->variables.at(1)));

	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(10), *obs_prep->states.at(13), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(11), *obs_prep->states.at(14), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(13), *obs_prep->states.at(14), retire_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(13), *obs_prep->states.at(0), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(14), *obs_prep->states.at(5), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(14), *obs_prep->states.at(4), free_function, *obs_prep->variables.at(1)));

	obs_prep->transitions.push_back(mk_transition_invocation_self(*obs_prep->states.at(13), *obs_prep->states.at(15), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_response_self(*obs_prep->states.at(15), *obs_prep->states.at(10), transfer_protect_function, *obs_prep->variables.at(0)));
	obs_prep->transitions.push_back(mk_transition_invocation_self(*obs_prep->states.at(14), *obs_prep->states.at(16), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_response_self(*obs_prep->states.at(16), *obs_prep->states.at(11), transfer_protect_function, *obs_prep->variables.at(0)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(15), *obs_prep->states.at(16), retire_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_any_thread(*obs_prep->states.at(16), *obs_prep->states.at(4), free_function, *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(15), *obs_prep->states.at(13), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(16), *obs_prep->states.at(14), transfer_protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(15), *obs_prep->states.at(0), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));
	obs_prep->transitions.push_back(mk_transition_invocation_self_neg(*obs_prep->states.at(16), *obs_prep->states.at(5), protect_function, *obs_prep->variables.at(0), *obs_prep->variables.at(1)));


	auto obs_safe = std::make_unique<Observer>();
	obs_safe->negative_specification = false;

	// variables
	obs_safe->variables.push_back(std::make_unique<ThreadObserverVariable>("HP-PT:thread"));
	obs_safe->variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>("HP-PT:address", ptrtype, false)));

	// states
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.0", true, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.1", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.2", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.3", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.4", false, true));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.5", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.6", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.7", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.8", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.9", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.10", false, true));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.11", false, true));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.12", false, false));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.13", false, true));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.14", false, true));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.15", false, true));
	obs_safe->states.push_back(std::make_unique<State>("HP-PT.16", false, true));

	// transitions for transfer_protect_function protection
	obs_safe->transitions.push_back(mk_transition_invocation_self(*obs_safe->states.at(0), *obs_safe->states.at(1), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_response_self(*obs_safe->states.at(1), *obs_safe->states.at(2), transfer_protect_function, *obs_safe->variables.at(0)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(2), *obs_safe->states.at(3), retire_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(3), *obs_safe->states.at(4), free_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self(*obs_safe->states.at(5), *obs_safe->states.at(6), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_response_self(*obs_safe->states.at(6), *obs_safe->states.at(7), transfer_protect_function, *obs_safe->variables.at(0)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(7), *obs_safe->states.at(3), retire_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(1), *obs_safe->states.at(0), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(2), *obs_safe->states.at(0), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(3), *obs_safe->states.at(5), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(6), *obs_safe->states.at(5), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(7), *obs_safe->states.at(5), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(0), *obs_safe->states.at(5), retire_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(1), *obs_safe->states.at(6), retire_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(5), *obs_safe->states.at(0), free_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(6), *obs_safe->states.at(1), free_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(7), *obs_safe->states.at(2), free_function, *obs_safe->variables.at(1)));

	// transitions for tranfering protection
	obs_safe->transitions.push_back(mk_transition_invocation_self(*obs_safe->states.at(2), *obs_safe->states.at(8), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self(*obs_safe->states.at(3), *obs_safe->states.at(9), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_response_self(*obs_safe->states.at(8), *obs_safe->states.at(10), protect_function, *obs_safe->variables.at(0)));
	obs_safe->transitions.push_back(mk_transition_response_self(*obs_safe->states.at(9), *obs_safe->states.at(11), protect_function, *obs_safe->variables.at(0)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(8), *obs_safe->states.at(9), retire_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(10), *obs_safe->states.at(11), retire_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(9), *obs_safe->states.at(4), free_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(11), *obs_safe->states.at(4), free_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(8), *obs_safe->states.at(0), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(9), *obs_safe->states.at(5), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(8), *obs_safe->states.at(2), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(10), *obs_safe->states.at(2), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(9), *obs_safe->states.at(3), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(11), *obs_safe->states.at(3), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));

	obs_safe->transitions.push_back(mk_transition_invocation_self(*obs_safe->states.at(7), *obs_safe->states.at(12), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(12), *obs_safe->states.at(9), retire_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(12), *obs_safe->states.at(5), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(12), *obs_safe->states.at(7), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(12), *obs_safe->states.at(8), free_function, *obs_safe->variables.at(1)));

	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(10), *obs_safe->states.at(13), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(11), *obs_safe->states.at(14), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(13), *obs_safe->states.at(14), retire_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(13), *obs_safe->states.at(0), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(14), *obs_safe->states.at(5), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(14), *obs_safe->states.at(4), free_function, *obs_safe->variables.at(1)));

	obs_safe->transitions.push_back(mk_transition_invocation_self(*obs_safe->states.at(13), *obs_safe->states.at(15), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_response_self(*obs_safe->states.at(15), *obs_safe->states.at(10), transfer_protect_function, *obs_safe->variables.at(0)));
	obs_safe->transitions.push_back(mk_transition_invocation_self(*obs_safe->states.at(14), *obs_safe->states.at(16), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_response_self(*obs_safe->states.at(16), *obs_safe->states.at(11), transfer_protect_function, *obs_safe->variables.at(0)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(15), *obs_safe->states.at(16), retire_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_any_thread(*obs_safe->states.at(16), *obs_safe->states.at(4), free_function, *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(15), *obs_safe->states.at(13), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(16), *obs_safe->states.at(14), transfer_protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(15), *obs_safe->states.at(0), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));
	obs_safe->transitions.push_back(mk_transition_invocation_self_neg(*obs_safe->states.at(16), *obs_safe->states.at(5), protect_function, *obs_safe->variables.at(0), *obs_safe->variables.at(1)));

	// ************************************************************************************************************************************* //
	// ************************************************************************************************************************************* //
	// ************************************************************************************************************************************* //
	// ************************************************************************************************************************************* //
	// ************************************************************************************************************************************* //
	// ************************************************************************************************************************************* //
	// ************************************************************************************************************************************* //
	// TODO: 
	// ************************************************************************************************************************************* //
	// ************************************************************************************************************************************* //

	result.push_back(std::move(obs_prep));
	result.push_back(std::move(obs_safe));
	return result;
}

std::unique_ptr<Observer> prtypes::make_last_free_observer(const Program& /*program*/, std::string /*name_prefix*/) {
	throw std::logic_error("not implemented");
	// auto [free_function, ptrtype] = get_basics();
	// assert(takes_single_pointer(free_function));

	// auto result = std::make_unique<Observer>();
	// result->negative_specification = false;

	// // variables
	// result->variables.push_back(std::make_unique<ThreadObserverVariable>(name_prefix + ":thread"));
	// result->variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>(name_prefix + ":address", ptrtype, false)));

	// // states
	// result->states.push_back(std::make_unique<State>(name_prefix + ".0", true, false));
	// result->states.push_back(std::make_unique<State>(name_prefix + ".1", false, true));
	// result->states.push_back(std::make_unique<State>(name_prefix + ".2", false, false));

	// // free transitions
	// result->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(0), *result->states.at(1), free_function, *result->variables.at(1)));

	// // subsequent transitions
	// auto add_trans = [&](const Function& label, Transition::Kind kind) {
	// 	result->transitions.push_back(std::make_unique<Transition>(*result->states.at(1), *result->states.at(2), label, kind, std::make_unique<TrueGuard>()));
	// };
	// add_trans(free_function, Transition::INVOCATION);
	// // add_trans(free_function, Transition::RESPONSE); // free has only invocations by design
	// for (const auto& function : program.functions) {
	// 	if (function->kind == Function::SMR) {
	// 		for (auto kind : { Transition::INVOCATION, Transition::RESPONSE }) {
	// 			add_trans(*function, kind);
	// 		}
	// 	}
	// }

	// return result;
}
