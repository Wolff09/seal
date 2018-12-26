#include "util/util.hpp"


struct CopyExpressionVisitor final : public BaseVisitor {
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

};

std::unique_ptr<Expression> cola::copy(const Expression& expr) {
	CopyExpressionVisitor visitor;
	expr.accept(visitor);
	return std::move(visitor.result);
}
