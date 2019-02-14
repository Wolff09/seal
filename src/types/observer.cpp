#include "types/observer.hpp"
#include "types/assumption.hpp"
#include <iostream>

using namespace cola;
using namespace prtypes;


SmrObserverStore::SmrObserverStore(const cola::Function& retire_function) : retire_function(retire_function), base_observer(prtypes::make_base_smr_observer(retire_function)) {
	this->simulation.compute_simulation(*this->base_observer);
	// TODO: show in the paper that base_observer does not need to support elision part (iii)?
	prtypes::raise_if_assumption_unsatisfied(*this->base_observer);
	// conditionally_raise_error<UnsupportedObserverError>(!supports_elison(*this->base_observer), "does not support elision");
}

void SmrObserverStore::add_impl_observer(std::unique_ptr<cola::Observer> observer) {
	prtypes::raise_if_assumption_unsatisfied(*observer);
	this->impl_observer.push_back(std::move(observer));
	this->simulation.compute_simulation(*this->impl_observer.back());
	conditionally_raise_error<UnsupportedObserverError>(!supports_elison(*this->impl_observer.back()), "does not support elision");
}

bool SmrObserverStore::supports_elison(const Observer& observer) const {
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
