#pragma once
#ifndef PRTYPES_OBSERVER
#define PRTYPES_OBSERVER

#include <memory>
#include <vector>
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "types/simulation.hpp"
#include "z3++.h"


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



	struct SymbolicState;
	struct SymbolicObserver;
	using SymbolicStateSet = std::set<const SymbolicState*>;

	struct SymbolicTransition {
		const SymbolicState& dst;
		const cola::Function& label;
		cola::Transition::Kind kind;
		z3::expr guard;

		// SymbolicTransition(SymbolicObserver& observer, const SymbolicState& dst, const cola::Transition& transition);
		SymbolicTransition(const SymbolicState& dst, const cola::Function& label, cola::Transition::Kind kind, z3::expr guard);
	};

	struct SymbolicState {
		const SymbolicObserver& observer;
		std::vector<std::unique_ptr<SymbolicTransition>> transitions;
		bool is_final, is_active;
		SymbolicStateSet closure;
		std::vector<const cola::State*> origin; // TODO: use sets?

		SymbolicState(const SymbolicObserver& observer, bool is_final, bool is_active);
	};

	struct SymbolicObserver {
		private:
			mutable z3::context context;
			mutable z3::solver solver;
			friend SymbolicStateSet symbolic_post(const SymbolicState& state, const cola::Command& command, const cola::VariableDeclaration& variable);

		public:
			std::vector<std::unique_ptr<SymbolicState>> states;
			z3::expr threadvar, adrvar, selfparam;
			std::vector<z3::expr> paramvars;

			SymbolicObserver(const SmrObserverStore& store);
	};


	SymbolicStateSet symbolic_post(const SymbolicState& state, const cola::Command& command, const cola::VariableDeclaration& variable); // TODO: observer should be const? => make solver a mutable private member and post a friend?

	SymbolicStateSet symbolic_post(const SymbolicStateSet& set, const cola::Command& command, const cola::VariableDeclaration& variable); // TODO: observer should be const? => make solver a mutable private member and post a friend?

	SymbolicStateSet symbolic_closure(const SymbolicStateSet& set);

	SymbolicStateSet symbolic_closure(const SymbolicState& state);


} // namespace prtypes

#endif
