#include "types/check.hpp"
#include "types/inference.hpp"

using namespace cola;
using namespace prtypes;


inline void raise_type_error(std::string cause) {
	throw std::logic_error("Type check error: " + cause + ".");
}

bool entails_valid(const GuaranteeSet& guarantees) {
	for (const auto& guarantee : guarantees) {
		if (guarantee.get().entails_validity) {
			return true;
		}
	}
	return false;
}

void remove_transient_guarantees(GuaranteeSet& guarantees) {
	for (auto it = guarantees.cbegin(); it != guarantees.cend(); ) {
		if (it->get().is_transient) {
			it = guarantees.erase(it); // progresses it to the next element
		} else {
			++it;
		}
	}
}


struct SubTypeCheckVisitor : public Visitor {
	TypeEnv type_environment;
	SubTypeCheckVisitor(TypeEnv&& env) : type_environment(std::move(env)) {}
	TypeEnv&& apply_typing(const AstNode& node) {
		node.accept(*this);
		return std::move(type_environment);
	}
};

struct TypeCheckEqCondVisitor : public SubTypeCheckVisitor {
	using SubTypeCheckVisitor::SubTypeCheckVisitor;
	bool in_lhs = true;
	const VariableDeclaration* lhs;
	const VariableDeclaration* rhs;

	virtual void do_type_check() = 0;

	void check_validity() {
		for (auto ptr : { lhs, rhs }) {
			if (ptr) {
				assert(type_environment.count(*ptr) > 0);
				if (!entails_valid(type_environment.at(*ptr))) {
					raise_type_error("comparison with potentially unsafe pointer");
				}
			} else {
				// NULL assumed to be valid // TODO: is this correct?
			}
		}
	}

	void update_typing() {
		// no type updates if one of the compared expressions is NULL // TODO: is this correct?
		if (!lhs || !rhs) {
			return;
		}

		// update types
		assert(type_environment.count(*lhs) > 0);
		assert(type_environment.count(*rhs) > 0);
		for (const auto& guarantee : type_environment.at(*lhs)) {
			type_environment.at(*rhs).insert(guarantee);
		}
		for (const auto& guarantee : type_environment.at(*rhs)) {
			type_environment.at(*lhs).insert(guarantee);
		}
	}

	void visit(const BinaryExpression& expression) override {
		assert(expression.lhs);
		assert(expression.rhs);

		// abort if no influence on type environment
		if (expression.sort() != Sort::PTR) {
			return;
		}
		if (expression.op != BinaryExpression::Operator::EQ) {
			return;
		}

		// expression is of the form: e1 == e2
		// query subexpressions e1 and e2 (only pointer variables and NULL are supported)
		in_lhs = true;
		expression.lhs->accept(*this);
		in_lhs = false;
		expression.rhs->accept(*this);

		// type check
		do_type_check();
	}

	void visit(const VariableExpression& expression) override {
		if (in_lhs) {
			lhs = &expression.decl;
		} else {
			rhs = &expression.decl;
		}
	}
	void visit(const NullValue& /*expression*/) override {
		// do nothing
	}
	void visit(const Dereference& /*dereference*/) override {
		// TODO: do we want/need this?
		throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Dereference&)");
	}

	virtual void visit(const VariableDeclaration& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const VariableDeclaration&)"); }
	virtual void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Expression&)"); }
	virtual void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const BooleanValue&)"); }
	virtual void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const EmptyValue&)"); }
	virtual void visit(const NDetValue& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const NDetValue&)"); }
	virtual void visit(const NegatedExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const NegatedExpression&)"); }
	virtual void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const InvariantExpression&)"); }
	virtual void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const InvariantActive&)"); }
	virtual void visit(const Sequence& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Sequence&)"); }
	virtual void visit(const Scope& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Scope&)"); }
	virtual void visit(const Atomic& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Atomic&)"); }
	virtual void visit(const Choice& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Choice&)"); }
	virtual void visit(const IfThenElse& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const IfThenElse&)"); }
	virtual void visit(const Loop& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Loop&)"); }
	virtual void visit(const While& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const While&)"); }
	virtual void visit(const Skip& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Skip&)"); }
	virtual void visit(const Break& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Break&)"); }
	virtual void visit(const Continue& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Continue&)"); }
	virtual void visit(const Assert& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Assert&)"); }
	virtual void visit(const Assume& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Assume&)"); }
	virtual void visit(const Return& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Return&)"); }
	virtual void visit(const Malloc& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Malloc&)"); }
	virtual void visit(const Assignment& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Assignment&)"); }
	virtual void visit(const Enter& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Enter&)"); }
	virtual void visit(const Exit& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Exit&)"); }
	virtual void visit(const Macro& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Macro&)"); }
	virtual void visit(const CompareAndSwap& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const CompareAndSwap&)"); }
	virtual void visit(const Function& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Function&)"); }
	virtual void visit(const Program& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckExpressionVisitor::visit(const Program&)"); }
};

struct TypeCheckAssumeVisitor : public TypeCheckEqCondVisitor {
	using TypeCheckEqCondVisitor::TypeCheckEqCondVisitor;
	void do_type_check() override {
		check_validity();
		update_typing();
	}

	void visit(const Assume& assume) override {
		assert(assume.expr);
		assume.expr->accept(*this);
	}
};

struct TypeCheckAssertExpressionVisitor : public TypeCheckEqCondVisitor {
	using TypeCheckEqCondVisitor::TypeCheckEqCondVisitor;
	void do_type_check() override {
		update_typing();
	}

	void visit(const Assert& assert) override {
		assert(assert.inv);
		assert.inv->accept(*this);
	}

	void visit(const InvariantExpression& invariant) override {
		assert(invariant.expr);
		invariant.expr->accept(*this);
	}
};


struct TypeCheckStatementVisitor final : public Visitor {
	const Program* current_program;
	const Function* current_function;
	TypeEnv current_type_environment;
	bool in_atomic = false; // handle commands occuring outside an atomic block as if they were wrapped in one

	bool is_well_typed(const Program& program) {
		program.accept(*this);
		throw std::logic_error("not yet implemented (TypeCheckVisitor::is_well_typed(const Program&)");
	}

	void visit(const Scope& scope) override {
		// populate current_type_environment with empty guarantees for declared pointer variables (prevent hiding)
		for (const auto& decl : scope.variables) {
			auto insertion = current_type_environment.insert({ *decl, GuaranteeSet() });
			if (!insertion.second) {
				raise_type_error("hiding variable declaration of outer scope not supported");
			}
		}

		// type check body
		if (scope.body) {
			scope.body->accept(*this);
		}

		// remove added type bindings
		for (const auto& decl : scope.variables) {
			current_type_environment.erase(*decl);
		}
	}

	void visit(const Sequence& sequence) override {
		assert(sequence.first);
		assert(sequence.second);
		sequence.first->accept(*this);
		sequence.second->accept(*this);
		// widening is applied directly at the commands
	}

	void visit(const Atomic& atomic) override {
		// prevent nesting atomic blocks
		if (in_atomic) {
			raise_type_error("nested 'atomic' blocks are not supported");
		}

		// type check body
		in_atomic = true;
		assert(atomic.body);
		atomic.body->accept(*this);
		in_atomic = false;

		// remove transient guarantees local pointers, remove all guarantees from shared pointers
		for (auto& [decl, guarantees] : current_type_environment) {
			if (decl.get().is_shared) {
				guarantees.clear();
			} else {
				remove_transient_guarantees(guarantees);
			}
		}
	}

	void visit(const Assume& /*node*/) override { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Assume&))"); }
	void visit(const Assert& /*node*/) override { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Assert&))"); }
	void visit(const Malloc& /*node*/) override { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Malloc&))"); }
	void visit(const Assignment& /*node*/) override { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Assignment&))"); }
	void visit(const Choice& /*node*/) override { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Choice&))"); }
	void visit(const Loop& /*node*/) override { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Loop&))"); }
	void visit(const Enter& /*node*/) override { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Enter&))"); }
	void visit(const Exit& /*node*/) override { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Exit&))"); }

	void visit(const Skip& /*node*/) override {
		// do nothing
	}
	void visit(const Return& /*node*/) override {
		// do nothing
	}

	void visit(const IfThenElse& /*node*/) override {
		raise_type_error("unsupported statement 'if'");
	}
	void visit(const While& /*node*/) override {
		raise_type_error("unsupported statement 'while'");
	}
	void visit(const Break& /*node*/) override {
		raise_type_error("unsupported statement 'break'");
	}
	void visit(const Continue& /*node*/) override {
		raise_type_error("unsupported statement 'continue'");
	}
	void visit(const Macro& /*node*/) override {
		raise_type_error("macros calls are not supported");
	}
	void visit(const CompareAndSwap& /*node*/) override {
		raise_type_error("unsupported statement 'CAS'");
	}

	void visit(const Function& function) override {
		switch (function.kind) {
			case Function::Kind::INTERFACE:
				break;

			case Function::Kind::MACRO:
				raise_type_error("unsupported function kind 'MACRO'"); // TODO: support this or assume inlineing?
				break;

			case Function::Kind::SMR:
				return; // no type check (has no implementation)
		}

		current_function = &function;
		current_type_environment.clear();
		function.body->accept(*this);
	}

	void visit(const Program& program) override {
		current_program = &program;
		for (const auto& function : program.functions) {
			function->accept(*this);
		}
	}

	void visit(const VariableDeclaration& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const VariableDeclaration&)"); }
	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const EmptyValue&)"); }
	void visit(const NDetValue& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const NDetValue&)"); }
	void visit(const VariableExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const VariableExpression&)"); }
	void visit(const NegatedExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const BinaryExpression&)"); }
	void visit(const Dereference& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const Dereference&)"); }
	void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const InvariantActive&)"); }
};



bool prtypes::type_check(const cola::Program& /*program*/, std::set<std::reference_wrapper<const Guarantee>> /*guarantees*/) {
	throw std::logic_error("not yet implemented (prtypes::type_check)");
}




////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
#include "cola/ast.hpp"
#include "cola/observer.hpp"

using namespace cola;
using namespace prtypes;

void print_guarantees(const GuaranteeSet set) {
	std::cout << "{";
	for (const auto& g : set) {
		std::cout << g.get().name << /*"[" << &(g.get()) << "]" <<*/ ", ";
	}
	std::cout << "}" << std::endl;
}

std::unique_ptr<Transition> mk_transition(const State& src, const State& dst, const Function& func, const ObserverVariable& obsvar) {
	auto guard = std::make_unique<EqGuard>(obsvar, std::make_unique<ArgumentGuardVariable>(*func.args.at(0)));
	return std::make_unique<Transition>(src, dst, func, Transition::INVOCATION, std::move(guard));
}

void prtypes::test() {
	std::cout << std::endl << std::endl << "Testing..." << std::endl;
	const Type& ptrtype = Observer::free_function().args.at(0)->type;

	Program program;
	program.name = "TestProgram";
	program.functions.push_back(std::make_unique<Function>("retire", Type::void_type(), Function::SMR));
	Function& retire = *program.functions.at(0);
	retire.args.push_back(std::make_unique<VariableDeclaration>("ptr", ptrtype, false));

	// active observer (simplified)
	auto obs_active_ptr = std::make_unique<Observer>();
	auto& obs_active = *obs_active_ptr;
	obs_active.negative_specification = false;
	obs_active.variables.push_back(std::make_unique<ThreadObserverVariable>("T"));
	obs_active.variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>("P", ptrtype, false)));

	obs_active.states.push_back(std::make_unique<State>("A.active", true, true));
	obs_active.states.push_back(std::make_unique<State>("A.retired", false, false));
	obs_active.states.push_back(std::make_unique<State>("A.dfreed", false, false));

	obs_active.transitions.push_back(mk_transition(*obs_active.states.at(0), *obs_active.states.at(1), retire, *obs_active.variables.at(1)));
	obs_active.transitions.push_back(mk_transition(*obs_active.states.at(1), *obs_active.states.at(0), Observer::free_function(), *obs_active.variables.at(1)));
	obs_active.transitions.push_back(mk_transition(*obs_active.states.at(0), *obs_active.states.at(2), Observer::free_function(), *obs_active.variables.at(1)));

	// safe observer (simplified)
	auto obs_safe_ptr = std::make_unique<Observer>();
	auto& obs_safe = *obs_safe_ptr;
	obs_safe.negative_specification = false;
	obs_safe.variables.push_back(std::make_unique<ThreadObserverVariable>("T"));
	obs_safe.variables.push_back(std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>("P", ptrtype, false)));

	obs_safe.states.push_back(std::make_unique<State>("S.init", true, true));
	obs_safe.states.push_back(std::make_unique<State>("S.retired", true, true));
	obs_safe.states.push_back(std::make_unique<State>("S.freed", false, false));

	obs_safe.transitions.push_back(mk_transition(*obs_safe.states.at(0), *obs_safe.states.at(1), retire, *obs_safe.variables.at(1)));
	obs_safe.transitions.push_back(mk_transition(*obs_safe.states.at(1), *obs_safe.states.at(2), Observer::free_function(), *obs_safe.variables.at(1)));


	// guarantees
	Guarantee gactive("active", std::move(obs_active_ptr));
	Guarantee gsafe("safe", std::move(obs_safe_ptr));

	GuaranteeSet gall = { gactive, gsafe };
	std::cout << "guarantees: ";
	print_guarantees(gall);

	// inference
	InferenceEngine ie(program, gall);

	// -> epsilon
	std::cout << "infer epsilon: ";
	auto inferEpsilon = ie.infer(gall);
	print_guarantees(inferEpsilon);
	assert(inferEpsilon.count(gactive) && inferEpsilon.count(gsafe));

	// -> enter
	Enter enter(retire);
	VariableDeclaration ptr("ptr", ptrtype, false);
	enter.args.push_back(std::make_unique<VariableExpression>(ptr));
	std::cout << "infer enter:retire(ptr): " << std::flush;
	auto inferEnter = ie.infer_enter(gall, enter, ptr);
	print_guarantees(inferEnter);
	assert(inferEnter.count(gactive) && inferEnter.count(gsafe));
}
