#include "types/check/check_impl.hpp"
#include "types/inference.hpp"

using namespace cola;
using namespace prtypes;


struct TypeCheckEqCondVisitor : public Visitor {
	const GuaranteeTable& guarantee_table;
	TypeEnv type_environment;

	bool in_lhs = true;
	const VariableDeclaration* lhs;
	const VariableDeclaration* rhs;

	TypeCheckEqCondVisitor(const GuaranteeTable& table, TypeEnv&& env) : guarantee_table(table), type_environment(std::move(env)) {}

	TypeEnv&& get_post_type_environment(const AstNode& node) {
		node.accept(*this);
		return std::move(type_environment);
	}

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
		type_environment.at(*lhs).erase(guarantee_table.local_guarantee());
		type_environment.at(*rhs).erase(guarantee_table.local_guarantee());
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
	bool has_condition = false;

	void do_type_check() override {
		if (has_condition) {
			update_typing();
		} else {
			throw std::logic_error("Unexpected invocation: TypeCheckAssertExpressionVisitor::do_type_check()");
		}
	}

	void visit(const Assert& assert) override {
		assert(assert.inv);
		assert.inv->accept(*this);
	}

	void visit(const InvariantExpression& invariant) override {
		has_condition = true;
		assert(invariant.expr);
		invariant.expr->accept(*this);
	}

	virtual void visit(const InvariantActive& invariant) override {
		has_condition = false;
		in_lhs = true;
		assert(invariant.expr);
		invariant.expr->accept(*this);

		if (lhs) {
			assert(type_environment.count(*lhs) > 0);
			type_environment.at(*lhs).insert(guarantee_table.active_guarantee());

		} else {
			raise_type_error("'active' invariant for 'NULL' not supported");
		}
	}
};


void TypeCheckStatementVisitor::visit(const Assume& assume) {
	TypeCheckAssumeVisitor visitor(this->guarantee_table, std::move(this->current_type_environment));
	this->current_type_environment = visitor.get_post_type_environment(assume);
}

void TypeCheckStatementVisitor::visit(const Assert& assert) {
	TypeCheckAssumeVisitor visitor(this->guarantee_table, std::move(this->current_type_environment));
	this->current_type_environment = visitor.get_post_type_environment(assert);
}
