#include "types/check.hpp"
#include "types/checker.hpp"
#include "types/assumption.hpp"

using namespace cola;

//
// base observer
//

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

std::unique_ptr<Observer> make_base_smr_observer(const Function& retire_function) {
	auto [free_function, ptrtype] = get_basics();
	assert(takes_single_pointer(free_function));
	assert(takes_single_pointer(retire_function));
	std::string name_prefix = "Base";

	auto result = std::make_unique<Observer>("BaseSMR");
	result->negative_specification = true;
	
	// variables
	result->variables.push_back(std::make_unique<ThreadObserverVariable>(name_prefix + ":thread"));
	result->variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>(name_prefix + ":address", ptrtype, false)));

	// states
	result->states.push_back(std::make_unique<State>(name_prefix + ".0", true, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".1", false, false));
	result->states.push_back(std::make_unique<State>(name_prefix + ".2", false, true));

	// transitions
	result->states.at(0)->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(0), *result->states.at(1), retire_function, *result->variables.at(1)));
	result->states.at(0)->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(0), *result->states.at(2), free_function, *result->variables.at(1)));
	result->states.at(1)->transitions.push_back(mk_transition_invocation_any_thread(*result->states.at(1), *result->states.at(0), free_function, *result->variables.at(1)));

	return result;
}


//
// SMR store
//

prtypes::SmrObserverStore::SmrObserverStore(const Program& program, const Function& retire_function) : program(program), retire_function(retire_function), base_observer(make_base_smr_observer(retire_function)) {
	this->simulation.compute_simulation(*this->base_observer);
	prtypes::raise_if_assumption_unsatisfied(*this->base_observer);
	// conditionally_raise_error<UnsupportedObserverError>(!supports_elison(*this->base_observer), "does not support elision");
	// NOTE: shown manually that base_observer supports elision?
}

void prtypes::SmrObserverStore::add_impl_observer(std::unique_ptr<Observer> observer) {
	prtypes::raise_if_assumption_unsatisfied(*observer);
	this->impl_observer.push_back(std::move(observer));
	this->simulation.compute_simulation(*this->impl_observer.back());
	conditionally_raise_error<UnsupportedObserverError>(!supports_elison(*this->impl_observer.back()), "does not support elision");
}

bool prtypes::SmrObserverStore::supports_elison(const Observer& observer) const {
	// requires simulation to be computed

	prtypes::raise_if_assumption_unsatisfied(observer);
	// assumption ==> free(<unobserved>) has not effect
	// assumption ==> swapping <unobserved> addresses has no effect
	// remains to check: <any state> in simulation relation with <initial state>

	for (const auto& state : observer.states) {
		if (state->initial) {
			for (const auto& other : observer.states) {
				if (!simulation.is_in_simulation_relation(*other, *state)) {
					return false;
				}
			}	
		}
	}
	return true;
}


//
// type check
//

bool prtypes::type_check(const cola::Program& program, const SmrObserverStore& observer_store) {
	TypeContext context(observer_store);
	TypeChecker checker(program, context);
	return checker.is_well_typed();
}
