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




	struct SymbolicState;
	struct SymbolicObserver;
	using SymbolicStateSet = std::set<const SymbolicState*>;

	struct SymbolicTransition {
		const SymbolicState& dst;
		const cola::Function& label;
		cola::Transition::Kind kind;
		z3::expr guard;

		SymbolicTransition(const SymbolicState& dst, const cola::Transition& transition);
	};

	struct SymbolicState {
		const SymbolicObserver& observer;
		std::vector<std::unique_ptr<SymbolicTransition>> transitions;
		bool is_final, is_active;
		SymbolicStateSet closure;
		std::vector<const cola::State*> origin;

		SymbolicState(const SymbolicObserver& observer, bool is_final, bool is_active);
	};

	struct SymbolicObserver {
		std::vector<std::unique_ptr<SymbolicState>> states;
		z3::context context;
		z3::solver solver;
		z3::expr threadvar, adrvar, selfparam;
		std::vector<z3::expr> paramvars;

		SymbolicObserver(const SmrObserverStore& store);
	};



	// TODO: constructors: transitionen vervollständigen, guards übersetzen


	SymbolicStateSet symbolic_post(const SymbolicState& state, const cola::Command& command, const cola::VariableDeclaration& variable);

	SymbolicStateSet symbolic_post(const SymbolicStateSet& set, const cola::Command& command, const cola::VariableDeclaration& variable);

	SymbolicStateSet symbolic_closure(const SymbolicStateSet& set);

	SymbolicStateSet symbolic_closure(const SymbolicState& state);


} // namespace prtypes

#endif
