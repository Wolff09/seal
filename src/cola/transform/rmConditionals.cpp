#include "cola/transform.hpp"

#include "cola/ast.hpp"
#include "cola/util.hpp"

using namespace cola;


struct IsTrueConditionVisitor final : public Visitor {
	bool result = false;
	void visit(const VariableDeclaration& /*node*/) override {}
	void visit(const Expression& /*node*/) override {}
	void visit(const BooleanValue& node) override { if (node.value) { result = true; } }
	void visit(const NullValue& /*node*/) override {}
	void visit(const EmptyValue& /*node*/) override {}
	void visit(const MaxValue& /*node*/) override {}
	void visit(const MinValue& /*node*/) override {}
	void visit(const NDetValue& /*node*/) override {}
	void visit(const VariableExpression& /*node*/) override {}
	void visit(const NegatedExpression& /*node*/) override {}
	void visit(const BinaryExpression& /*node*/) override {}
	void visit(const Dereference& /*node*/) override {}
	void visit(const InvariantExpression& /*node*/) override {}
	void visit(const InvariantActive& /*node*/) override {}
	void visit(const Sequence& /*node*/) override {}
	void visit(const Scope& /*node*/) override {}
	void visit(const Atomic& /*node*/) override {}
	void visit(const Choice& /*node*/) override {}
	void visit(const IfThenElse& /*node*/) override {}
	void visit(const Loop& /*node*/) override {}
	void visit(const While& /*node*/) override {}
	void visit(const Skip& /*node*/) override {}
	void visit(const Break& /*node*/) override {}
	void visit(const Continue& /*node*/) override {}
	void visit(const Assume& /*node*/) override {}
	void visit(const Assert& /*node*/) override {}
	void visit(const Return& /*node*/) override {}
	void visit(const Malloc& /*node*/) override {}
	void visit(const Assignment& /*node*/) override {}
	void visit(const Enter& /*node*/) override {}
	void visit(const Exit& /*node*/) override {}
	void visit(const Macro& /*node*/) override {}
	void visit(const CompareAndSwap& /*node*/) override {}
	void visit(const Function& /*node*/) override {}
	void visit(const Program& /*node*/) override {}
};

struct RemoveConditionalsVisitor final : public NonConstVisitor {
	std::unique_ptr<Statement> replacement;
	bool replacementNeeded = false;

	void visit(Program& program) {
		program.initializer->accept(*this);
		for (auto& func : program.functions) {
			func->accept(*this);
		}
	}

	void visit(Function& function) {
		if (function.body) {
			function.body->accept(*this);
		}
	}

	void visit(Atomic& atomic) {
		atomic.body->accept(*this);
	}

	void visit(Choice& choice) {
		for (auto& scope : choice.branches) {
			scope->accept(*this);
		}
	}
	void visit(Loop& loop) {
		loop.body->accept(*this);
	}

	void prependAssumption(Scope& scope, std::unique_ptr<Expression> expr) {
		auto assume = std::make_unique<Assume>(std::move(expr));
		auto seq = std::make_unique<Sequence>(std::move(assume), std::move(scope.body));
		scope.body = std::move(seq);
	}

	void visit(IfThenElse& ite) {
		if (ite.elseBranch) {
			ite.elseBranch->accept(*this);
			auto expr = cola::negate(*ite.expr);
			auto cast = dynamic_cast<const Skip*>(ite.elseBranch->body.get()); // TODO: less hacky way
			if (cast) {
				ite.elseBranch->body = std::make_unique<Assume>(std::move(expr));
			} else {
				prependAssumption(*ite.elseBranch, std::move(expr));
			}
		}

		ite.ifBranch->accept(*this);
		prependAssumption(*ite.ifBranch, std::move(ite.expr));

		auto choice = std::make_unique<Choice>();
		choice->branches.reserve(2);
		choice->branches.push_back(std::move(ite.ifBranch));
		choice->branches.push_back(std::move(ite.elseBranch));
		
		replacement = std::move(choice);
		replacementNeeded = true;
	}

	void visit(While& whl) {
		whl.body->accept(*this);

		IsTrueConditionVisitor visitor;
		whl.expr->accept(visitor);
		if (!visitor.result) {
			// rewrite if whl.expr is not 'true'
			auto expr = cola::negate(*whl.expr);
			prependAssumption(*whl.body, std::move(whl.expr));
			auto loop = std::make_unique<Loop>(std::move(whl.body));
			auto assume = std::make_unique<Assume>(std::move(expr));
			auto seq = std::make_unique<Sequence>(std::move(loop), std::move(assume));

			replacement = std::move(seq);
			replacementNeeded = true;
		}
	}

	void acceptAndReplaceIfNeeded(std::unique_ptr<Statement>& uptr) {
		uptr->accept(*this);
		if (replacementNeeded) {
			assert(replacement);
			uptr = std::move(replacement);
			replacement.reset();
			replacementNeeded = false;
		} 
	}

	void visit(Scope& scope) {
		acceptAndReplaceIfNeeded(scope.body);
	}

	void visit(Sequence& sequence) {
		acceptAndReplaceIfNeeded(sequence.first);
		acceptAndReplaceIfNeeded(sequence.second);
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

	void visit(VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(VariableDeclaration&))"); }
	void visit(Expression& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(Expression&))"); }
	void visit(BooleanValue& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(BooleanValue&))"); }
	void visit(NullValue& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(NullValue&))"); }
	void visit(EmptyValue& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(EmptyValue&))"); }
	void visit(MaxValue& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(MaxValue&))"); }
	void visit(MinValue& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(MinValue&))"); }
	void visit(NDetValue& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(NDetValue&))"); }
	void visit(VariableExpression& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(VariableExpression&))"); }
	void visit(NegatedExpression& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(NegatedExpression&))"); }
	void visit(BinaryExpression& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(BinaryExpression&))"); }
	void visit(Dereference& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(Dereference&))"); }
	void visit(InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(InvariantExpression&))"); }
	void visit(InvariantActive& /*node*/) { throw std::logic_error("Unexpected invokation (RemoveConditionalsVisitor::visit(InvariantActive&))"); }
};


void cola::remove_conditionals(Program& program) {
	RemoveConditionalsVisitor().visit(program);
}
