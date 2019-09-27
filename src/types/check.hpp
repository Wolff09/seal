#pragma once
#ifndef PRTYPES_CHECK
#define PRTYPES_CHECK

#include <optional>
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "types/simulation.hpp"


namespace prtypes {

	struct SmrObserverStore {
		const cola::Program& program;
		const cola::Function& retire_function;
		std::unique_ptr<cola::Observer> base_observer;
		std::vector<std::unique_ptr<cola::Observer>> impl_observer;
		SimulationEngine simulation; // TODO: should this be exposed here?

		SmrObserverStore(const cola::Program& program, const cola::Function& retire_function);
		
		bool supports_elison(const cola::Observer& observer) const;
		
		void add_impl_observer(std::unique_ptr<cola::Observer> observer);
	};


	bool type_check(const cola::Program& program, const SmrObserverStore& observer_store);

} // namespace prtypes

#endif