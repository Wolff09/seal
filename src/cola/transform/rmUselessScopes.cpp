#include "cola/transform.hpp"

using namespace cola;


struct RemoveUselessScopeVisitor final : NonConstVisitor {
	bool is_scope = false;
	Scope* sub_scope;

	void flatten(std::unique_ptr<Statement>& dst) {
		dst->accept(*this);

		if (is_scope) {
			assert(sub_scope);
			assert(dst.get() == sub_scope);
			if (sub_scope->variables.size() != 0) {
				throw std::logic_error("Rewrite error: cannot remove scope, contains variable declaration.");
			}
			dst = std::move(sub_scope->body);
		}

		is_scope = false;
	}

	void handle_scope(Scope& scope, bool useless) {
		flatten(scope.body);

		is_scope = useless;
		sub_scope = &scope;
	}

	void visit(Program& program) {
		program.initializer->accept(*this);
		for (auto& func : program.functions) {
			func->accept(*this);
		}
	}

	void visit(Function& function) {
		if (function.body) {
			handle_scope(*function.body, false);
		}
	}

	void visit(Scope& scope) {
		// by construction: this scope is useless
		handle_scope(scope, true);
	}

	void visit(Sequence& sequence) {
		flatten(sequence.first);
		flatten(sequence.second);
	}

	void visit(Atomic& atomic) {
		handle_scope(*atomic.body, false);
	}

	void visit(Choice& choice) {
		for (auto& scope : choice.branches) {
			handle_scope(*scope, false);
		}
	}

	void visit(IfThenElse& ite) {
		handle_scope(*ite.ifBranch, false);
		handle_scope(*ite.elseBranch, false);
	}

	void visit(Loop& loop) {
		handle_scope(*loop.body, false);
	}

	void visit(While& whl) {
		handle_scope(*whl.body, false);
	}

	void visit(Skip& /*node*/) { /* do nothing */ }
	void visit(Break& /*node*/) { /* do nothing */ }
	void visit(Continue& /*node*/) { /* do nothing */ }
	void visit(Assume& /*node*/) { /* do nothing */ }
	void visit(Assert& /*node*/) { /* do nothing */ }
	void visit(Return& /*node*/) { /* do nothing */ }
	void visit(Malloc& /*node*/) { /* do nothing */ }
	void visit(Assignment& /*node*/) { /* do nothing */ }
	void visit(Enter& /*node*/) { /* do nothing */ }
	void visit(Exit& /*node*/) { /* do nothing */ }
	void visit(Macro& /*node*/) { /* do nothing */ }
	void visit(CompareAndSwap& /*node*/) { /* do nothing */ }

	void visit(VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(VariableDeclaration&))"); }
	void visit(Expression& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(Expression&))"); }
	void visit(BooleanValue& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(BooleanValue&))"); }
	void visit(NullValue& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(NullValue&))"); }
	void visit(EmptyValue& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(EmptyValue&))"); }
	void visit(NDetValue& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(NDetValue&))"); }
	void visit(VariableExpression& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(VariableExpression&))"); }
	void visit(NegatedExpression& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(NegatedExpression&))"); }
	void visit(BinaryExpression& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(BinaryExpression&))"); }
	void visit(Dereference& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(Dereference&))"); }
	void visit(InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(InvariantExpression&))"); }
	void visit(InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(InvariantActive&))"); }
};


void cola::remove_useless_scopes(Program& program) {
	RemoveUselessScopeVisitor().visit(program);
}
