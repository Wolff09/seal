#include "types/check.hpp"
#include "types/error.hpp"

using namespace cola;
using namespace prtypes;


struct TypeCheckBaseVisitor : public Visitor {
	virtual void visit(const VariableDeclaration& /*node*/) override { /* do nothing */ }
	virtual void visit(const Expression& /*node*/) override { /* do nothing */ }
	virtual void visit(const BooleanValue& /*node*/) override { /* do nothing */ }
	virtual void visit(const NullValue& /*node*/) override { /* do nothing */ }
	virtual void visit(const EmptyValue& /*node*/) override { /* do nothing */ }
	virtual void visit(const NDetValue& /*node*/) override { /* do nothing */ }
	virtual void visit(const VariableExpression& /*node*/) override { /* do nothing */ }
	virtual void visit(const NegatedExpression& /*node*/) override { /* do nothing */ }
	virtual void visit(const BinaryExpression& /*node*/) override { /* do nothing */ }
	virtual void visit(const Dereference& /*node*/) override { /* do nothing */ }
	virtual void visit(const InvariantExpression& /*node*/) override { /* do nothing */ }
	virtual void visit(const InvariantActive& /*node*/) override { /* do nothing */ }
	virtual void visit(const Sequence& /*node*/) override { /* do nothing */ }
	virtual void visit(const Scope& /*node*/) override { /* do nothing */ }
	virtual void visit(const Atomic& /*node*/) override { /* do nothing */ }
	virtual void visit(const Choice& /*node*/) override { /* do nothing */ }
	virtual void visit(const IfThenElse& /*node*/) override { /* do nothing */ }
	virtual void visit(const Loop& /*node*/) override { /* do nothing */ }
	virtual void visit(const While& /*node*/) override { /* do nothing */ }
	virtual void visit(const Skip& /*node*/) override { /* do nothing */ }
	virtual void visit(const Break& /*node*/) override { /* do nothing */ }
	virtual void visit(const Continue& /*node*/) override { /* do nothing */ }
	virtual void visit(const Assume& /*node*/) override { /* do nothing */ }
	virtual void visit(const Assert& /*node*/) override { /* do nothing */ }
	virtual void visit(const Return& /*node*/) override { /* do nothing */ }
	virtual void visit(const Malloc& /*node*/) override { /* do nothing */ }
	virtual void visit(const Assignment& /*node*/) override { /* do nothing */ }
	virtual void visit(const Enter& /*node*/) override { /* do nothing */ }
	virtual void visit(const Exit& /*node*/) override { /* do nothing */ }
	virtual void visit(const Macro& /*node*/) override { /* do nothing */ }
	virtual void visit(const CompareAndSwap& /*node*/) override { /* do nothing */ }
	virtual void visit(const Function& /*node*/) override { /* do nothing */ }
	virtual void visit(const Program& /*node*/) override { /* do nothing */ }
};

struct VariableExpressionVisitor final : public TypeCheckBaseVisitor {
	const VariableDeclaration* decl = nullptr;
	void visit(const VariableExpression& var) override { this->decl = &var.decl; }
};

struct VariableExpressionOrDereferenceOrNullValueVisitor final : public TypeCheckBaseVisitor {
	const VariableDeclaration* decl = nullptr;
	const Dereference* deref = nullptr;
	const NullValue* null = nullptr;
	void visit(const VariableExpression& var) override { this->decl = &var.decl; }
	void visit(const NullValue& null) override { this->null = &null; }
	void visit(const Dereference& deref) override { this->deref = &deref; }
};

struct BinaryExpressionVisitor final : public TypeCheckBaseVisitor {
	const BinaryExpression* binary;
	void visit(const BinaryExpression& expr) override { this->binary = &expr; }
};


const VariableDeclaration& TypeChecker::expression_to_variable(const Expression& expression) {
	VariableExpressionVisitor visitor;
	expression.accept(visitor);
	conditionally_raise_error<UnsupportedConstructError>(!visitor.decl, "unsupported expression; expected a variable");
	assert(visitor.decl);
	return *visitor.decl;
}

TypeChecker::VariableOrDereferenceOrNull TypeChecker::expression_to_variable_or_dereference_or_null(const Expression& expression) {
	VariableExpressionOrDereferenceOrNullValueVisitor visitor;
	expression.accept(visitor);
	conditionally_raise_error<UnsupportedConstructError>(!visitor.decl && !visitor.deref && !visitor.null, "unsupported expression; expected a variable, a dereference, or 'NULL'");
	assert(!!visitor.decl ^ !!visitor.deref ^ !!visitor.null);
	TypeChecker::VariableOrDereferenceOrNull result;
	if (visitor.decl) {
		result.var = visitor.decl;
	}
	if (visitor.deref) {
		result.deref = visitor.deref;
	}
	if (visitor.null) {
		result.null = visitor.null;
	}
	return result;
}

TypeChecker::FlatBinaryExpression TypeChecker::expression_to_flat_binary_expression(const Expression& expression) {
	BinaryExpressionVisitor visitor;
	expression.accept(visitor);
	conditionally_raise_error<UnsupportedConstructError>(!visitor.binary, "unsupported expression; expected a binary expression");
	assert(visitor.binary);
	assert(visitor.binary->lhs);
	assert(visitor.binary->rhs);
	TypeChecker::FlatBinaryExpression result;
	result.lhs = &this->expression_to_variable(*visitor.binary->lhs);
	result.op = visitor.binary->op;
	result.rhs = this->expression_to_variable_or_dereference_or_null(*visitor.binary->rhs);
	return result;
}


void TypeChecker::visit(const VariableDeclaration& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const VariableDeclaration&)");
}

void TypeChecker::visit(const Expression& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Expression&)");
}

void TypeChecker::visit(const BooleanValue& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const BooleanValue&)");
}

void TypeChecker::visit(const NullValue& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const NullValue&)");
}

void TypeChecker::visit(const EmptyValue& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const EmptyValue&)");
}

void TypeChecker::visit(const NDetValue& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const NDetValue&)");
}

void TypeChecker::visit(const VariableExpression& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const VariableExpression&)");
}

void TypeChecker::visit(const NegatedExpression& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const NegatedExpression&)");
}

void TypeChecker::visit(const BinaryExpression& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const BinaryExpression&)");
}

void TypeChecker::visit(const Dereference& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Dereference&)");
}


void TypeChecker::visit(const Sequence& sequence) {
	this->check_sequence(sequence);
}

void TypeChecker::visit(const Scope& scope) {
	this->check_scope(scope);
}

void TypeChecker::visit(const Atomic& atomic) {
	conditionally_raise_error<UnsupportedConstructError>(inside_atomic, "nested atomic blocks are not supported");
	inside_atomic = true;
	this->check_atomic(atomic);
	inside_atomic = false;
}

void TypeChecker::visit(const Choice& choice) {
	this->check_choice(choice);
}

void TypeChecker::visit(const IfThenElse& /*node*/) {
	raise_error<UnsupportedConstructError>("'if' statements are not supported");
}

void TypeChecker::visit(const Loop& loop) {
	this->check_loop(loop);
}

void TypeChecker::visit(const While& /*node*/) {
	raise_error<UnsupportedConstructError>("'while' statements are not supported");
}

void TypeChecker::visit(const Skip& skip) {
	this->check_skip(skip);
}

void TypeChecker::visit(const Break& /*node*/) {
	raise_error<UnsupportedConstructError>("'break' statements are not supported");
}

void TypeChecker::visit(const Continue& /*node*/) {
	raise_error<UnsupportedConstructError>("'continue' statements are not supported");
}

void TypeChecker::visit(const Assume& assume) {
	assert(assume.expr);
	if (assume.expr->sort() == Sort::PTR) {
		auto flat = expression_to_flat_binary_expression(*assume.expr);
		conditionally_raise_error<UnsupportedConstructError>(flat.rhs.deref.has_value(), "dereferences within conditions are not supported");
		assert(flat.rhs.var.has_value() ^ flat.rhs.null.has_value());
		if (flat.rhs.var.has_value()) {
			this->check_assume_pointer(assume, *flat.lhs, flat.op, **flat.rhs.var);
		} else {
			this->check_assume_pointer(assume, *flat.lhs, flat.op, **flat.rhs.null);
		}

	} else {
		this->check_assume_nonpointer(assume, *assume.expr);
	}
}

void TypeChecker::visit(const Assert& assrt) {
	this->current_assert = &assrt;
	assert(assrt.inv);
	assrt.inv->accept(*this);
	this->current_assert = nullptr;
}

void TypeChecker::visit(const InvariantExpression& invariant) {
	assert(invariant.expr);
	if (invariant.expr->sort() == Sort::PTR) {
		auto flat = expression_to_flat_binary_expression(*invariant.expr);
		conditionally_raise_error<UnsupportedConstructError>(flat.rhs.deref.has_value(), "dereferences within conditions are not supported");
		assert(flat.rhs.var.has_value() ^ flat.rhs.null.has_value());
		if (flat.rhs.var.has_value()) {
			this->check_assert_pointer(*this->current_assert, *flat.lhs, flat.op, **flat.rhs.var);
		} else {
			this->check_assert_pointer(*this->current_assert, *flat.lhs, flat.op, **flat.rhs.null);
		}
	
	} else {
		raise_error<UnsupportedConstructError>("assertions with data expressions are not supported");
	}
}

void TypeChecker::visit(const InvariantActive& invariant) {
	assert(invariant.expr);
	this->check_assert_active(*this->current_assert, expression_to_variable(*invariant.expr));
}

void TypeChecker::visit(const Return& retrn) {
	assert(retrn.expr);
	expression_to_variable(*retrn.expr);
}

void TypeChecker::visit(const Malloc& malloc) {
	this->check_malloc(malloc, malloc.lhs);
}

void TypeChecker::visit(const Assignment& assignment) {
	assert(assignment.lhs);
	assert(assignment.rhs);

	auto lhs = expression_to_variable_or_dereference_or_null(*assignment.lhs);
	auto rhs = expression_to_variable_or_dereference_or_null(*assignment.rhs);

	conditionally_raise_error<UnsupportedConstructError>(lhs.null.has_value(), "cannot assign to 'NULL'");
	conditionally_raise_error<UnsupportedConstructError>(lhs.deref.has_value() && rhs.deref.has_value(), "simultanious dereference on left-hand-side and right-hand-side of assignment are not supported");
	assert(lhs.var.has_value() ^ lhs.deref.has_value());
	assert(rhs.var.has_value() ^ rhs.deref.has_value() ^ rhs.null.has_value());

	if (lhs.var.has_value()) {
		if (rhs.var.has_value()) {
			check_assign_pointer(assignment, **lhs.var, **rhs.var);
		} else if (rhs.deref.has_value()) {
			assert((*rhs.deref)->expr);
			auto& deref_var = expression_to_variable(*(*rhs.deref)->expr);
			check_assign_pointer(assignment, **lhs.var, **rhs.deref, deref_var);
		} else {
			assert(rhs.null.has_value());
			check_assign_pointer(assignment, **lhs.var, **rhs.null);
		}
		
	} else {
		assert((*lhs.deref)->expr);
		auto& deref_var = expression_to_variable(*(*lhs.deref)->expr);
		if (rhs.var.has_value()) {
			check_assign_pointer(assignment, **lhs.deref, deref_var, **rhs.var);
		} else {
			assert(rhs.null.has_value());
			check_assign_pointer(assignment, **lhs.deref, deref_var, **rhs.null);
		}
	}
}

void TypeChecker::visit(const Enter& enter) {
	std::vector<std::reference_wrapper<const VariableDeclaration>> params;
	for (const auto& arg : enter.args) {
		assert(arg);
		params.push_back(expression_to_variable(*arg));
	}

	this->check_enter(enter, std::move(params));
}

void TypeChecker::visit(const Exit& exit) {
	this->check_exit(exit);
}

void TypeChecker::visit(const Macro& /*node*/) {
	raise_error<UnsupportedConstructError>("calls to 'MACRO' functions are not supported");
}

void TypeChecker::visit(const CompareAndSwap& /*node*/) {
	raise_error<UnsupportedConstructError>("CAS statements are not supported");
}

void TypeChecker::visit(const Function& function) {
	switch (function.kind) {
		case Function::Kind::INTERFACE:
			this->check_interface_function(function);
			return;

		case Function::Kind::MACRO:
			raise_error<UnsupportedConstructError>("unsupported function kind 'MACRO'"); // TODO: support this or assume inlineing?
			return;

		case Function::Kind::SMR:
			return; // has no implementation (no type check)
	}
}

void TypeChecker::visit(const Program& program) {
	this->check_program(program);
}
