#include "util/util.hpp"


struct CopyExpressionVisitor final : public Visitor {
	std::unique_ptr<Expression> result;

	void visit(const BooleanValue& expr) {
		assert(!result);
		result = std::make_unique<BooleanValue>(expr.value);
	}

	void visit(const NullValue& /*expr*/) {
		assert(!result);
		result = std::make_unique<NullValue>();
	}

	void visit(const EmptyValue& /*expr*/) {
		assert(!result);
		result = std::make_unique<EmptyValue>();
	}

	void visit(const NDetValue& /*expr*/) {
		assert(!result);
		result = std::make_unique<EmptyValue>();
	}

	void visit(const VariableExpression& expr) {
		assert(!result);
		result = std::make_unique<VariableExpression>(expr.decl);
	}

	void visit(const NegatedExpression& expr) {
		assert(!result);
		expr.expr->accept(*this);
		result = std::make_unique<NegatedExpression>(std::move(result));
	}

	void visit(const BinaryExpression& expr) {
		assert(!result);
		expr.lhs->accept(*this);
		std::unique_ptr<Expression> lhs = std::move(result);
		assert(!result);
		expr.rhs->accept(*this);
		std::unique_ptr<Expression> rhs = std::move(result);
		result = std::make_unique<BinaryExpression>(expr.op, std::move(lhs), std::move(rhs));
	}

	void visit(const Dereference& expr) {
		assert(!result);
		expr.expr->accept(*this);
		result = std::make_unique<Dereference>(std::move(result), expr.fieldname);
	}

	void visit(const VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const VariableDeclaration&))"); }
	void visit(const Expression& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Expression&))"); }
	void visit(const InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const InvariantExpression&))"); }
	void visit(const InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const InvariantActive&))"); }
	void visit(const Sequence& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Sequence&))"); }
	void visit(const Scope& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Scope&))"); }
	void visit(const Atomic& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Atomic&))"); }
	void visit(const Choice& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Choice&))"); }
	void visit(const IfThenElse& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const IfThenElse&))"); }
	void visit(const Loop& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Loop&))"); }
	void visit(const While& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const While&))"); }
	void visit(const Skip& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Skip&))"); }
	void visit(const Break& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Break&))"); }
	void visit(const Continue& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Continue&))"); }
	void visit(const Assume& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Assume&))"); }
	void visit(const Assert& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Assert&))"); }
	void visit(const Return& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Return&))"); }
	void visit(const Malloc& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Malloc&))"); }
	void visit(const Assignment& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Assignment&))"); }
	void visit(const Enter& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Enter&))"); }
	void visit(const Exit& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Exit&))"); }
	void visit(const Macro& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Macro&))"); }
	void visit(const CompareAndSwap& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const CompareAndSwap&))"); }
	void visit(const Function& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Function&))"); }
	void visit(const Program& /*node*/) { throw std::logic_error("Unexpected invocation (CopyExpressionVisitor::visit(const Program&))"); }

};

std::unique_ptr<Expression> cola::copy(const Expression& expr) {
	CopyExpressionVisitor visitor;
	expr.accept(visitor);
	return std::move(visitor.result);
}
