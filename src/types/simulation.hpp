#pragma once
#ifndef PRTYPES_SIMULATION
#define PRTYPES_SIMULATION


#include <set>
#include "cola/observer.hpp"

namespace prtypes {

	class SimulationEngine {
		public:
			using SimulationRelation = std::set<std::pair<const cola::State*, const cola::State*>>;

		private:
			SimulationRelation all_simulations;
			std::set<const cola::Observer*> observers;

		public:
			void compute_simulation(const cola::Observer& observer);
			bool is_in_simulation_relation(const cola::State& state, const cola::State& other);
			// TODO: export function to compute the post states wrt. to a guard and validity ==> or just offer the safe predice here
	};

} // namespace prtypes

#endif