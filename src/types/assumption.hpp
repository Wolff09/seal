#pragma once
#ifndef PRTYPES_ASSUMPTION
#define PRTYPES_ASSUMPTION

#include <memory>
#include "cola/observer.hpp"
#include "types/error.hpp"


namespace prtypes {

	struct ObserverAssumptionCheckVisitor : public cola::ObserverVisitor {
		std::size_t count_address = 0;
		std::size_t count_thread = 0;
		bool malformed = false;
		std::string cause;
		const cola::Observer* observer;
		bool label_is_free = false;
		bool comparison_neq = false;
		bool address_is_observed = false;
		bool contains_var(const cola::ObserverVariable& var) {
			assert(observer);
			bool found = false;
			for (const auto& other : observer->variables) {
				if (other.get() == &var) {
					found = true;
					break;
				}
			}
			return found;
		}
		void visit(const cola::ThreadObserverVariable& var) {
			assert(contains_var(var));
			prtypes::conditionally_raise_error<std::logic_error>(!contains_var(var), "Unexpected failure in ObserverAssumptionCheckVisitor.");
			++count_thread;
		}
		void visit(const cola::ProgramObserverVariable& var) {
			assert(contains_var(var));
			prtypes::conditionally_raise_error<std::logic_error>(!contains_var(var), "Unexpected failure in ObserverAssumptionCheckVisitor.");
			++count_address;
			if (label_is_free) {
				if (comparison_neq) {
					malformed = true;
					cause = "pointer arguments to function 'free' must be observed";
				} else {
					address_is_observed = true;
				}
			}
		}
		void visit(const cola::SelfGuardVariable& /*obj*/) { /* do nothing */ }
		void visit(const cola::ArgumentGuardVariable& /*obj*/) { /* do nothing */ }
		void visit(const cola::TrueGuard& /*obj*/) { /* do nothing */ }
		void visit(const cola::ConjunctionGuard& guard) {
			for (const auto& subguard : guard.conjuncts) {
				subguard->accept(*this);
			}
		}
		void visit(const cola::EqGuard& guard) {
			comparison_neq = false;
			guard.lhs.accept(*this);
			guard.rhs->accept(*this);
		}
		void visit(const cola::NeqGuard& guard) {
			comparison_neq = true;
			guard.lhs.accept(*this);
			guard.rhs->accept(*this);
		}
		void visit(const cola::State& /*obj*/) { throw std::logic_error("Unexpected invocation: ObserverAssumptionCheckVisitor::visit(const State&)"); }
		void visit(const cola::Transition& transition) {
			label_is_free = &transition.label == &cola::Observer::free_function();
			address_is_observed = false;
			if (label_is_free && transition.kind != cola::Transition::INVOCATION) {
				malformed = true;
				cause = "function 'free' must only occur as enter event";
				return;
			}
			transition.guard->accept(*this);
			if (label_is_free && ! address_is_observed) {
				malformed = true;
				cause = "pointer arguments to function 'free' must be observed";
			}
		}
		void visit(const cola::Observer& observer) {
			this->observer = &observer;
			for (const auto& var : observer.variables) {
				var->accept(*this);
			}
			if (count_address != 1) {
				cause = "observers must have exactly one observer variable of pointer type (Sort::PTR), have " + std::to_string(count_address);
				malformed = true;
				return;
			}
			if (count_thread != 1) {
				cause = "observers must have exactly one observer variable for threads, have " + std::to_string(count_thread);
				malformed = true;
				return;
			}
			for (const auto& state : observer.states) {
				for (const auto& transition : state->transitions) {
					transition->accept(*this);
					if (malformed) {
						return;
					}
				}
			}
		}
	};

	inline bool satisfies_assumption(const cola::Observer& observer) {
		ObserverAssumptionCheckVisitor visitor;
		observer.accept(visitor);
		return !visitor.malformed;
	}

	inline void raise_if_assumption_unsatisfied(const cola::Observer& observer) {
		ObserverAssumptionCheckVisitor visitor;
		observer.accept(visitor);
		if (visitor.malformed) {
			prtypes::raise_error<UnsupportedObserverError>(visitor.cause);
		}
	}

} // namespace prtypes

#endif