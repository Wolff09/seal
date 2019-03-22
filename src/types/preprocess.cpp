#include "types/preprocess.hpp"
#include "cola/util.hpp"
#include "cola/transform.hpp"
#include "types/error.hpp"
#include <iostream>
#include <set>

using namespace cola;
using namespace prtypes;


struct CollectDerefVisitor final : public Visitor {
	std::vector<const Dereference*> derefs;
	bool is_assume = true;

	void visit(const VariableDeclaration& /*node*/) { /* do nothing */ }
	void visit(const BooleanValue& /*node*/) { /* do nothing */ }
	void visit(const NullValue& /*node*/) { /* do nothing */ }
	void visit(const EmptyValue& /*node*/) { /* do nothing */ }
	void visit(const MaxValue& /*node*/) { /* do nothing */ }
	void visit(const MinValue& /*node*/) { /* do nothing */ }
	void visit(const NDetValue& /*node*/) { /* do nothing */ }
	void visit(const VariableExpression& /*node*/) { /* do nothing */ }
	void visit(const NegatedExpression& node) {
		assert(node.expr);
		node.expr->accept(*this);
	}
	void visit(const BinaryExpression& node) {
		assert(node.lhs);
		node.lhs->accept(*this);
		assert(node.rhs);
		node.rhs->accept(*this);
	}
	void visit(const Dereference& node) {
		if (node.sort() == Sort::PTR) {
			this->derefs.push_back(&node);
		}
	}

	void visit(const Assume& node) {
		assert(node.expr);
		node.expr->accept(*this);
	}
	
	void visit(const Expression& /*node*/) { throw std::logic_error("Unexpected invocation: CollectDerefVisitor::visit(const Expression&)"); }
	void visit(const InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: CollectDerefVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: CollectDerefVisitor::visit(const InvariantActive&)"); }
	void visit(const Sequence& /*node*/) { is_assume = false; }
	void visit(const Scope& /*node*/) { is_assume = false; }
	void visit(const Atomic& /*node*/) { is_assume = false; }
	void visit(const Choice& /*node*/) { is_assume = false; }
	void visit(const IfThenElse& /*node*/) { is_assume = false; }
	void visit(const Loop& /*node*/) { is_assume = false; }
	void visit(const While& /*node*/) { is_assume = false; }
	void visit(const Skip& /*node*/) { is_assume = false; }
	void visit(const Break& /*node*/) { is_assume = false; }
	void visit(const Continue& /*node*/) { is_assume = false; }
	void visit(const Assert& /*node*/) { is_assume = false; }
	void visit(const Return& /*node*/) { is_assume = false; }
	void visit(const Malloc& /*node*/) { is_assume = false; }
	void visit(const Assignment& /*node*/) { is_assume = false; }
	void visit(const Enter& /*node*/) { is_assume = false; }
	void visit(const Exit& /*node*/) { is_assume = false; }
	void visit(const Macro& /*node*/) { is_assume = false; }
	void visit(const CompareAndSwap& /*node*/) { is_assume = false; }
	void visit(const Function& /*node*/) { throw std::logic_error("Unexpected invocation: CollectDerefVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) { throw std::logic_error("Unexpected invocation: CollectDerefVisitor::visit(const Program&)"); }
};

struct LocalExpressionVisitor final : public Visitor {
	bool result = true;
	std::set<const VariableDeclaration*> decls;
	void visit(const VariableDeclaration& decl) {
		if (decl.is_shared) {
			result = false;
		}
		decls.insert(&decl);
	}
	void visit(const Dereference& deref) {
		// memory access may not be local
		assert(deref.expr);
		deref.expr->accept(*this);
		result = false;
	}
	void visit(const BooleanValue& /*node*/) { /* do nothing */ }
	void visit(const NullValue& /*node*/) { /* do nothing */ }
	void visit(const EmptyValue& /*node*/) { /* do nothing */ }
	void visit(const MaxValue& /*node*/) { /* do nothing */ }
	void visit(const MinValue& /*node*/) { /* do nothing */ }
	void visit(const NDetValue& /*node*/) { /* do nothing */ }
	void visit(const VariableExpression& expr) {
		expr.decl.accept(*this);
	}
	void visit(const NegatedExpression& expr) {
		assert(expr.expr);
		expr.expr->accept(*this);
	}
	void visit(const BinaryExpression& expr) {
		assert(expr.lhs);
		assert(expr.rhs);
		expr.lhs->accept(*this);
		expr.rhs->accept(*this);
	}
	
	void visit(const Expression& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Expression&)"); }
	void visit(const InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const InvariantActive&)"); }
	void visit(const Sequence& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Sequence&)"); }
	void visit(const Scope& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Scope&)"); }
	void visit(const Atomic& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Atomic&)"); }
	void visit(const Choice& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Choice&)"); }
	void visit(const IfThenElse& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const IfThenElse&)"); }
	void visit(const Loop& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Loop&)"); }
	void visit(const While& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const While&)"); }
	void visit(const Skip& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Skip&)"); }
	void visit(const Break& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Break&)"); }
	void visit(const Continue& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Continue&)"); }
	void visit(const Assume& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Assume&)"); }
	void visit(const Assert& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Assert&)"); }
	void visit(const Return& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Return&)"); }
	void visit(const Malloc& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Malloc&)"); }
	void visit(const Assignment& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Assignment&)"); }
	void visit(const Enter& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Enter&)"); }
	void visit(const Exit& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Exit&)"); }
	void visit(const Macro& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Macro&)"); }
	void visit(const CompareAndSwap& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const CompareAndSwap&)"); }
	void visit(const Function& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const Program&)"); }
};

inline bool is_expression_local(const Expression& expr) {
	LocalExpressionVisitor visitor;
	expr.accept(visitor);
	return visitor.result;
}

struct NeedsAtomicVisitor final : public Visitor {
	bool result = true;
	const Function& retire_function;
	NeedsAtomicVisitor(const Function& retire_function) : retire_function(retire_function) {}

	void visit(const VariableDeclaration& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const VariableDeclaration&)"); }
	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const EmptyValue&)"); }
	void visit(const MaxValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const MaxValue&)"); }
	void visit(const MinValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const MinValue&)"); }
	void visit(const NDetValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const NDetValue&)"); }
	void visit(const VariableExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const VariableExpression&)"); }
	void visit(const NegatedExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const BinaryExpression&)"); }
	void visit(const Dereference& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const Dereference&)"); }
	void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const InvariantActive&)"); }
	void visit(const Function& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const Program&)"); }

	void visit(const Sequence& /*node*/) override { /* do nothing */ }
	void visit(const Scope& /*node*/) override { /* do nothing */ }
	void visit(const Choice& /*node*/) override { /* do nothing */ }
	void visit(const IfThenElse& /*node*/) override { /* do nothing */ }
	void visit(const Loop& /*node*/) override { /* do nothing */ }
	void visit(const While& /*node*/) override { /* do nothing */ }
	void visit(const Return& /*node*/) override { /* do nothing */ }
	void visit(const Macro& /*node*/) override { /* do nothing */ }
	void visit(const CompareAndSwap& /*node*/) override { /* do nothing */ }

	void visit(const Atomic& /*node*/) override { this->result = false; }
	void visit(const Skip& /*node*/) override { this->result = false; }
	void visit(const Break& /*node*/) override { this->result = false; }
	void visit(const Continue& /*node*/) override { this->result = false; }
	void visit(const Assume& node) override {
		if (is_expression_local(*node.expr)) {
			this->result = false;
		}
	}
	void visit(const Assert& node) override {
		if (is_expression_local(*node.inv->expr)) {
			this->result = false;
		}
	}
	void visit(const Malloc& node) override {
		if (!node.lhs.is_shared) {
			this->result = false;
		}
	}
	void visit(const Assignment& node) override {
		if (is_expression_local(*node.lhs) && is_expression_local(*node.rhs)) {
			this->result = false;
		}
	}
	void visit(const Enter& node) override {
		if (&node.decl == &retire_function) {
			return;
		}
		for (const auto& arg : node.args) {
			if (!is_expression_local(*arg)) {
				return;
			}
		}
		this->result = false;
	}
	void visit(const Exit& /*node*/) override { this->result = false; }
};

bool needs_atomic(const Statement& stmt, const Function& retire_function) {
	NeedsAtomicVisitor visitor(retire_function);
	stmt.accept(visitor);
	return visitor.result;
}

struct PreprocessingVisitor final : public NonConstVisitor {
	bool in_atomic = false;
	bool found_return = false;
	bool found_cmd = false;
	const Function& retire_function;
	PreprocessingVisitor(const Function& retire_function) : retire_function(retire_function) {}

	void visit(VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(VariableDeclaration&)"); }
	void visit(Expression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(Expression&)"); }
	void visit(BooleanValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(BooleanValue&)"); }
	void visit(NullValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(NullValue&)"); }
	void visit(EmptyValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(EmptyValue&)"); }
	void visit(MaxValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(MaxValue&)"); }
	void visit(MinValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(MinValue&)"); }
	void visit(NDetValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(NDetValue&)"); }
	void visit(VariableExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(VariableExpression&)"); }
	void visit(NegatedExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(NegatedExpression&)"); }
	void visit(BinaryExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(BinaryExpression&)"); }
	void visit(Dereference& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(Dereference&)"); }
	void visit(InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(InvariantExpression&)"); }
	void visit(InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(InvariantActive&)"); }
	
	void visit(IfThenElse& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(IfThenElse&)"); }
	void visit(While& node) {
		auto& expr = *node.expr;
		assert(typeid(expr) == typeid(BooleanValue));
		node.body->accept(*this);
	}
	void visit(CompareAndSwap& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(CompareAndSwap&)"); }
	void visit(Continue& /*node*/) { raise_error<UnsupportedConstructError>("'continue' not supported"); }
	void visit(Macro& /*node*/) { raise_error<UnsupportedConstructError>("'continue' not supported"); }

	void handle_assume(std::unique_ptr<Statement>& stmt) {
		assert(stmt);
		CollectDerefVisitor visitor;
		stmt->accept(visitor);
		if (visitor.is_assume && !visitor.derefs.empty()) {
			cola::print(*stmt, std::cout);
			std::unique_ptr<Statement> result = std::move(stmt);
			for (const auto& deref : visitor.derefs) {
				assert(deref);
				auto assertion = std::make_unique<Assert>(std::make_unique<InvariantActive>(cola::copy(*deref)));
				result = std::make_unique<Sequence>(std::move(result), std::move(assertion));
			}
			stmt = std::move(result);
		}
		assert(stmt);
	}

	void handle_statement(std::unique_ptr<Statement>& stmt) {
		found_cmd = false;
		assert(stmt);
		stmt->accept(*this);
		handle_assume(stmt);
		if (found_cmd && !in_atomic && needs_atomic(*stmt, retire_function)) {
			stmt = std::make_unique<Atomic>(std::make_unique<Scope>(std::move(stmt)));
		}
		found_cmd = false;
	}

	void visit(Sequence& node) {
		assert(node.first);
		assert(node.second);
		handle_statement(node.first);
		conditionally_raise_error<UnsupportedConstructError>(found_return, "'return' must be the last statement in a function");
		handle_statement(node.second);
	}
	void visit(Scope& node) {
		assert(node.body);
		handle_statement(node.body);
	}
	void visit(Atomic& node) {
		assert(node.body);
		bool was_in_atomic = in_atomic;
		in_atomic = true;
		node.body->accept(*this);
		in_atomic = was_in_atomic;
		found_cmd = false;
	}
	void visit(Choice& node) {
		conditionally_raise_error<UnsupportedConstructError>(node.branches.size() == 1, "'choice' with only one branch indicates error; maybe you meant to add a branch containing just 'skip'");
		for (auto& branch : node.branches) {
			assert(branch);
			branch->accept(*this);
			conditionally_raise_error<UnsupportedConstructError>(found_return, "'return' must not occur inside a choice");
		}
	}
	void visit(Loop& node) {
		assert(node.body);
		node.body->accept(*this);
		conditionally_raise_error<UnsupportedConstructError>(found_return, "'return' must not occur inside a loop");
	}

	void visit(Break& /*node*/) { found_cmd = true; }
	void visit(Skip& /*node*/) { found_cmd = true; }
	void visit(Assume& /*node*/) { found_cmd = true; }
	void visit(Assert& /*node*/) { found_cmd = true; }
	void visit(Return& /*node*/) { found_cmd = true; found_return = true; }
	void visit(Malloc& /*node*/) { found_cmd = true; }
	void visit(Assignment& /*node*/) { found_cmd = true; }
	void visit(Enter& /*node*/) { found_cmd = true; }
	void visit(Exit& /*node*/) { found_cmd = true; }

	void visit(Function& node) {
		found_return = false;
		if (node.body) {
			node.body->accept(*this);
		}
	}
	void visit(Program& node) {
		// TODO: what about the initializer
		for (auto& function : node.functions) {
			assert(function);
			function->accept(*this);
		}
	}
};

void prtypes::preprocess(Program& program, const cola::Function& retire_function) {
	cola::remove_cas(program);
	cola::remove_scoped_variables(program);
	cola::remove_conditionals(program);
	cola::remove_useless_scopes(program);

	PreprocessingVisitor visitor(retire_function);
	program.accept(visitor);
}
