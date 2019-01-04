#pragma once
#ifndef COLA_OBSERVER
#define COLA_OBSERVER

#include <memory>
#include <vector>
#include <string>
#include "cola/ast.hpp"


namespace cola {

	struct State;
	struct ThreadObserverVariable;
	struct ProgramObserverVariable;
	struct SelfGuardVariable;
	struct ArgumentGuardVariable;
	struct TrueGuard;
	struct ConjunctionGuard;
	struct EqGuard;
	struct NeqGuard;
	struct Transition;
	struct Observer;

	struct ObserverVisitor {
		virtual void visit(const ThreadObserverVariable& obj) = 0;
		virtual void visit(const ProgramObserverVariable& obj) = 0;
		virtual void visit(const SelfGuardVariable& obj) = 0;
		virtual void visit(const ArgumentGuardVariable& obj) = 0;
		virtual void visit(const TrueGuard& obj) = 0;
		virtual void visit(const ConjunctionGuard& obj) = 0;
		virtual void visit(const EqGuard& obj) = 0;
		virtual void visit(const NeqGuard& obj) = 0;
		virtual void visit(const State& obj) = 0;
		virtual void visit(const Transition& obj) = 0;
		virtual void visit(const Observer& obj) = 0;
	};

	struct State {
		std::string name;
		bool initial = false;
		bool final = false;
		virtual ~State() = default;
		State() {}
		State(std::string name_) : name(name_) {}
		State(std::string name_, bool initial_, bool final_) : name(name_), initial(initial_), final(final_) {}
		virtual void accept(ObserverVisitor& visitor) const { visitor.visit(*this); }
	};

	struct ObserverVariable {
		virtual ~ObserverVariable() = default;
		virtual void accept(ObserverVisitor& visitor) const = 0;
	};

	struct ThreadObserverVariable : ObserverVariable {
		std::string name;
		ThreadObserverVariable(std::string name_) : name(name_) {}
		virtual void accept(ObserverVisitor& visitor) const override { visitor.visit(*this); }
	};

	struct ProgramObserverVariable : ObserverVariable {
		std::unique_ptr<VariableDeclaration> decl;
		ProgramObserverVariable(std::unique_ptr<VariableDeclaration> decl_) : decl(std::move(decl_)) {}
		virtual void accept(ObserverVisitor& visitor) const override { visitor.visit(*this); }
	};

	struct GuardVariable {
		virtual ~GuardVariable() = default;
		virtual void accept(ObserverVisitor& visitor) const = 0;
	};

	struct SelfGuardVariable : GuardVariable {
		virtual void accept(ObserverVisitor& visitor) const override { visitor.visit(*this); }
	};

	struct ArgumentGuardVariable : GuardVariable {
		const VariableDeclaration& decl;
		ArgumentGuardVariable(const VariableDeclaration& decl_) : decl(decl_) {}
		virtual void accept(ObserverVisitor& visitor) const override { visitor.visit(*this); }
	};

	struct Guard {
		virtual ~Guard() = default;
		virtual void accept(ObserverVisitor& visitor) const = 0;
	};

	struct TrueGuard : public Guard {
		virtual void accept(ObserverVisitor& visitor) const override { visitor.visit(*this); }
	};

	struct ConjunctionGuard : public Guard {
		std::unique_ptr<Guard> lhs;
		std::unique_ptr<Guard> rhs;
		ConjunctionGuard(std::unique_ptr<Guard> lhs_, std::unique_ptr<Guard> rhs_) : lhs(std::move(lhs_)), rhs(std::move(rhs_)) {}
		virtual void accept(ObserverVisitor& visitor) const override { visitor.visit(*this); }
	};

	struct ComparisonGuard : public Guard {
		const ObserverVariable& lhs;
		const GuardVariable& rhs;
		ComparisonGuard(const ObserverVariable& lhs_, const GuardVariable& rhs_) : lhs(lhs_), rhs(rhs_) {}
	};

	struct EqGuard : public ComparisonGuard {
		EqGuard(const ObserverVariable& lhs_, const GuardVariable& rhs_) : ComparisonGuard(lhs_, rhs_) {}
		virtual void accept(ObserverVisitor& visitor) const override { visitor.visit(*this); }
	};

	struct NeqGuard : public ComparisonGuard {
		NeqGuard(const ObserverVariable& lhs_, const GuardVariable& rhs_) : ComparisonGuard(lhs_, rhs_) {}
		virtual void accept(ObserverVisitor& visitor) const override { visitor.visit(*this); }
	};

	struct Transition {
		const State& src;
		const State& dst;
		const Function& label;
		std::unique_ptr<Guard> guard;
		virtual ~Transition() = default;
		Transition(const State& src_, const State& dst_, const Function& label_, std::unique_ptr<Guard> guard_) : src(src_), dst(dst_), label(label_), guard(std::move(guard_)) {}
		virtual void accept(ObserverVisitor& visitor) const { visitor.visit(*this); }
	};

	struct Observer {
		bool negative_specification = true; // negative specification accepts what is *not* in specification (classic notion)
		std::vector<std::unique_ptr<ObserverVariable>> variables;
		std::vector<std::unique_ptr<State>> states;
		std::vector<std::unique_ptr<Transition>> transitions;
		virtual ~Observer() = default;
		virtual void accept(ObserverVisitor& visitor) const { visitor.visit(*this); }
	};


} // namespace cola

#endif