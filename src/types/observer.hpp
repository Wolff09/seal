#pragma once
#ifndef PRTYPES_OBSERVER
#define PRTYPES_OBSERVER

#include "cola/observer.hpp"
#include "types/factory.hpp"
#include "types/error.hpp"
#include "types/assumption.hpp"
#include "types/simulation.hpp"


namespace prtypes {

	struct SmrObserverStore {
		const cola::Program& program;
		const cola::Function& retire_function;
		std::unique_ptr<cola::Observer> base_observer;
		std::vector<std::unique_ptr<cola::Observer>> impl_observer;
		SimulationEngine simulation;

		SmrObserverStore(const cola::Program& program, const cola::Function& retire_function);
		
		bool supports_elison(const cola::Observer& observer) const;
		
		void add_impl_observer(std::unique_ptr<cola::Observer> observer);
	};

} // namespace prtypes

#endif