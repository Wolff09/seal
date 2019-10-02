#include "cola/util.hpp"

using namespace cola;


struct CopyObserverVisitor final : public ObserverVisitor {
	std::unique_ptr<Observer> result;
	std::unique_ptr<GuardVariable> current_guard_variable;
	std::unique_ptr<Guard> current_guard;
	std::map<const ObserverVariable*, const ObserverVariable*> obsvar2copy;
	std::map<const State*, State*> state2copy;

	void visit(const ThreadObserverVariable& var) {
		result->variables.push_back(std::make_unique<ThreadObserverVariable>(var.name));
		obsvar2copy[&var] = result->variables.back().get();
	}

	void visit(const ProgramObserverVariable& var) {
		auto decl = std::make_unique<VariableDeclaration>(var.decl->name, var.decl->type, var.decl->is_shared);
		result->variables.push_back(std::make_unique<ProgramObserverVariable>(std::move(decl)));
		obsvar2copy[&var] = result->variables.back().get();
	}

	void visit(const SelfGuardVariable& /*var*/) {
		current_guard_variable = std::make_unique<SelfGuardVariable>();
	}

	void visit(const ArgumentGuardVariable& var) {
		current_guard_variable = std::make_unique<ArgumentGuardVariable>(var.decl);
	}

	void visit(const TrueGuard& /*guard*/) {
		current_guard = std::make_unique<TrueGuard>();
	}

	void visit(const ConjunctionGuard& guard) {
		std::vector<std::unique_ptr<Guard>> subguards;
		for (const auto& conjunct : guard.conjuncts) {
			conjunct->accept(*this);
			subguards.push_back(std::move(current_guard));
		}
		current_guard = std::make_unique<ConjunctionGuard>(std::move(subguards));
	}

	void visit(const EqGuard& guard) {
		guard.rhs->accept(*this);
		current_guard = std::make_unique<EqGuard>(*obsvar2copy.at(&guard.lhs), std::move(current_guard_variable));
	}

	void visit(const NeqGuard& guard) {
		guard.rhs->accept(*this);
		current_guard = std::make_unique<NeqGuard>(*obsvar2copy.at(&guard.lhs), std::move(current_guard_variable));
	}

	void visit(const State& state) {
		result->states.push_back(std::make_unique<State>(state.name, state.initial, state.final));
		state2copy[&state] = result->states.back().get();
		for (const auto& transition : state.transitions) {
			transition->accept(*this);
		}
	}

	void visit(const Transition& transition) {
		transition.guard->accept(*this);
		State& src_copy = *state2copy.at(&transition.src);
		src_copy.transitions.push_back(std::make_unique<Transition>(src_copy, *state2copy.at(&transition.dst), transition.label, transition.kind, std::move(current_guard)));
	}

	void visit(const Observer& observer) {
		result = std::make_unique<Observer>(observer.name);
		result->negative_specification = observer.negative_specification;
		for (const auto& var : observer.variables) {
			var->accept(*this);
		}
		for (const auto& state : observer.states) {
			state->accept(*this);
		}
	}
};

std::unique_ptr<Observer> cola::copy(const Observer& observer) {
	CopyObserverVisitor visitor;
	observer.accept(visitor);
	return std::move(visitor.result);
}

std::unique_ptr<ObserverVariable> cola::copy(const ObserverVariable& variable) {
	CopyObserverVisitor visitor;
	visitor.result = std::make_unique<Observer>("<dummy>");
	variable.accept(visitor);
	return std::move(visitor.result->variables.back());
}