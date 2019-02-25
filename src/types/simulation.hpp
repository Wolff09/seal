#pragma once
#ifndef PRTYPES_SIMULATION
#define PRTYPES_SIMULATION


#include <set>
#include <vector>
#include <functional>
#include "cola/ast.hpp"
#include "cola/observer.hpp"

namespace prtypes {

	class SimulationEngine {
		public:
			struct VariableDeclarationSetComparator {
				bool operator()(const std::reference_wrapper<const cola::VariableDeclaration>& lhs, const std::reference_wrapper<const cola::VariableDeclaration>& rhs) const {
					return std::less<const cola::VariableDeclaration*>()(&(lhs.get()), &(rhs.get()));
				}
			};
			using VariableDeclarationSet = std::set<std::reference_wrapper<const cola::VariableDeclaration>, VariableDeclarationSetComparator>;
			using SimulationRelation = std::set<std::pair<const cola::State*, const cola::State*>>;

		private:
			SimulationRelation all_simulations;
			std::set<const cola::Observer*> observers;

		public:
			void compute_simulation(const cola::Observer& observer);
			bool is_in_simulation_relation(const cola::State& state, const cola::State& other) const;
			bool is_safe(const cola::Enter& enter, const std::vector<std::reference_wrapper<const cola::VariableDeclaration>>& params, const VariableDeclarationSet& invalid_params) const;
			bool is_repeated_execution_simulating(const std::vector<std::reference_wrapper<const cola::Command>>& events) const;
	};

} // namespace prtypes

#endif