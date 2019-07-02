#include "cola/transform.hpp"

using namespace cola;


struct MoveVariablesVisitor final : NonConstVisitor {
	Function* currentFunction;
	std::size_t counter = 2;

	bool contains(const VariableDeclaration& decl) {
		assert(currentFunction);
		for (const auto& var : currentFunction->args) {
			if (var->name == decl.name) {
				return true;
			}
		}
		for (const auto& var : currentFunction->body->variables) {
			if (var->name == decl.name) {
				return true;
			}
		}
		return false;
	}

	void visit(Scope& scope) {
		// TODO: reuse temporary variables from unrelated loops
		assert(currentFunction);
		currentFunction->body->variables.reserve(currentFunction->body->variables.size() + scope.variables.size());
		for (auto& var : scope.variables) {
			if (contains(*var)) {
				var->name = var->name + "_" + std::to_string(counter++);
				if (contains(*var)) {
					throw std::logic_error("Name clash: cannot move variable with name '" + var->name + "', it exists in multiple loops; I tried to rename it but failed.");
				}
			}

			assert(!contains(*var));
			currentFunction->body->variables.push_back(std::move(var));
			assert(!var);
		}
		scope.variables.clear();
		scope.body->accept(*this);
	}

	void visit(Program& program) {
		program.initializer->accept(*this);
		for (auto& func : program.functions) {
			func->accept(*this);
		}
	}

	void visit(Function& function) {
		currentFunction = &function;
		if (function.body) {
			function.body->body->accept(*this); // do not visit top level scope
		}
	}

	void visit(Sequence& sequence) {
		sequence.first->accept(*this);
		sequence.second->accept(*this);
	}

	void visit(Atomic& atomic) {
		atomic.body->accept(*this);
	}

	void visit(Choice& choice) {
		for (auto& scope : choice.branches) {
			scope->accept(*this);
		}
	}

	void visit(IfThenElse& ite) {
		ite.ifBranch->accept(*this);
		ite.elseBranch->accept(*this);
	}

	void visit(Loop& loop) {
		loop.body->accept(*this);
	}

	void visit(While& whl) {
		whl.body->accept(*this);
	}

	void visit(Skip& /*node*/) { /* do nothing */ }
	void visit(Break& /*node*/) { /* do nothing */ }
	void visit(Continue& /*node*/) { /* do nothing */ }
	void visit(Assume& /*node*/) { /* do nothing */ }
	void visit(Assert& /*node*/) { /* do nothing */ }
	void visit(AngelChoose& /*node*/) { /* do nothing */ }
	void visit(AngelActive& /*node*/) { /* do nothing */ }
	void visit(AngelContains& /*node*/) { /* do nothing */ }
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
	void visit(MaxValue& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(MaxValue&))"); }
	void visit(MinValue& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(MinValue&))"); }
	void visit(NDetValue& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(NDetValue&))"); }
	void visit(VariableExpression& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(VariableExpression&))"); }
	void visit(NegatedExpression& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(NegatedExpression&))"); }
	void visit(BinaryExpression& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(BinaryExpression&))"); }
	void visit(Dereference& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(Dereference&))"); }
	void visit(InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(InvariantExpression&))"); }
	void visit(InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation (NonConstVisitor::visit(InvariantActive&))"); }
};


void cola::remove_scoped_variables(Program& program) {
	MoveVariablesVisitor().visit(program);
}
