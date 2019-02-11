#pragma once
#ifndef PRTYPES_SIMULATION
#define PRTYPES_SIMULATION


#include <set>
#include "cola/observer.hpp"

namespace prtypes {

	using SimulationPair = std::pair<std::reference_wrapper<const cola::State>, std::reference_wrapper<const cola::State>>;

	struct SimulationRelationComparator {
		bool operator() (const SimulationPair& lhs, const SimulationPair& rhs) const {
			std::pair<const cola::State*, const cola::State*> lhs_pair = { &(lhs.first.get()), &(lhs.second.get()) };
			std::pair<const cola::State*, const cola::State*> rhs_pair = { &(rhs.first.get()), &(rhs.second.get()) };
			return std::less()(lhs_pair, rhs_pair);
		}
	};

	using SimulationRelation = std::set<SimulationPair, SimulationRelationComparator>;


	SimulationRelation compute_simulation(const cola::Observer& observer);

} // namespace prtypes

#endif