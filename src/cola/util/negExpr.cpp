#include "cola/util.hpp"

using namespace cola;


BinaryExpression::Operator negate_binary_opereator(BinaryExpression::Operator op) {
	switch (op) {
		case BinaryExpression::Operator::EQ: return BinaryExpression::Operator::NEQ;
		case BinaryExpression::Operator::NEQ: return BinaryExpression::Operator::EQ;
		case BinaryExpression::Operator::LEQ: return BinaryExpression::Operator::GT;
		case BinaryExpression::Operator::LT: return BinaryExpression::Operator::GEQ;
		case BinaryExpression::Operator::GEQ: return BinaryExpression::Operator::LT;
		case BinaryExpression::Operator::GT: return BinaryExpression::Operator::LEQ;
		case BinaryExpression::Operator::AND: return BinaryExpression::Operator::OR;
		case BinaryExpression::Operator::OR: return BinaryExpression::Operator::AND;
	}
}

struct NegateExpressionVisitor final : public Visitor {
	std::unique_ptr<Expression> result;

	void visit(const BooleanValue& expr) {
		assert(!result);
		result = std::make_unique<BooleanValue>(expr.value);
	}

	void visit(const NegatedExpression& expr) {
		assert(!result);
		result = cola::copy(*expr.expr);
	}

	void visit(const BinaryExpression& expr) {
		assert(!result);
		auto inverted_op = negate_binary_opereator(expr.op);
		if (inverted_op == BinaryExpression::Operator::AND || inverted_op == BinaryExpression::Operator::OR) {
			expr.lhs->accept(*this);
			auto lhs = std::move(result);
			expr.rhs->accept(*this);
			auto rhs = std::move(result);
			result = std::make_unique<BinaryExpression>(inverted_op, std::move(lhs), std::move(rhs));
		} else {
			result = std::make_unique<BinaryExpression>(inverted_op, cola::copy(*expr.lhs), cola::copy(*expr.rhs));
		}
	}

	void visit(const VariableExpression& expr) {
		assert(expr.decl.type.sort == Sort::BOOL);
		assert(!result);
		result = std::make_unique<NegatedExpression>(cola::copy(expr));
	}

	void visit(const NullValue& /*expr*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const NullValue&))"); }
	void visit(const EmptyValue& /*expr*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const EmptyValue&))"); }
	void visit(const NDetValue& /*expr*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const NDetValue&))"); }
	void visit(const Dereference& /*expr*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Dereference&))"); }

	void visit(const VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const VariableDeclaration&))"); }
	void visit(const Expression& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Expression&))"); }
	void visit(const InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const InvariantExpression&))"); }
	void visit(const InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const InvariantActive&))"); }
	void visit(const Sequence& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Sequence&))"); }
	void visit(const Scope& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Scope&))"); }
	void visit(const Atomic& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Atomic&))"); }
	void visit(const Choice& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Choice&))"); }
	void visit(const IfThenElse& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const IfThenElse&))"); }
	void visit(const Loop& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Loop&))"); }
	void visit(const While& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const While&))"); }
	void visit(const Skip& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Skip&))"); }
	void visit(const Break& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Break&))"); }
	void visit(const Continue& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Continue&))"); }
	void visit(const Assume& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Assume&))"); }
	void visit(const Assert& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Assert&))"); }
	void visit(const Return& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Return&))"); }
	void visit(const Malloc& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Malloc&))"); }
	void visit(const Assignment& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Assignment&))"); }
	void visit(const Enter& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Enter&))"); }
	void visit(const Exit& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Exit&))"); }
	void visit(const Macro& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Macro&))"); }
	void visit(const CompareAndSwap& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const CompareAndSwap&))"); }
	void visit(const Function& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Function&))"); }
	void visit(const Program& /*node*/) { throw std::logic_error("Unexpected invocation (NegateExpressionVisitor::visit(const Program&))"); }

};

std::unique_ptr<Expression> cola::negate(const Expression& expr) {
		NegateExpressionVisitor visitor;
		expr.accept(visitor);
		return std::move(visitor.result);
}
