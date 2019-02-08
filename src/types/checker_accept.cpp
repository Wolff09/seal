#include "types/check.hpp"
#include "types/error.hpp"

using namespace cola;
using namespace prtypes;

const cola::VariableDeclaration& TypeChecker::expression_to_variable(const cola::Expression& /*expression*/) {
	throw std::logic_error("not yet implemented: TypeChecker::expression_to_variable(const cola::Expression&)");
}

TypeChecker::FlatBinaryExpression TypeChecker::expression_to_flat_binary_expression(const cola::Expression& /*expression*/) {
	throw std::logic_error("not yet implemented: TypeChecker::expression_to_flat_binary_expression(const cola::Expression&)");
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
	raise_unsupported_if(inside_atomic, "nested atomic blocks are not supported");
	inside_atomic = true;
	this->check_atomic(atomic);
	inside_atomic = false;
}

void TypeChecker::visit(const Choice& choice) {
	this->check_choice(choice);
}

void TypeChecker::visit(const IfThenElse& /*node*/) {
	raise_unsupported("'if' statements are not supported");
}

void TypeChecker::visit(const Loop& loop) {
	this->check_loop(loop);
}

void TypeChecker::visit(const While& /*node*/) {
	raise_unsupported("'while' statements are not supported");
}

void TypeChecker::visit(const Skip& skip) {
	this->check_skip(skip);
}

void TypeChecker::visit(const Break& /*node*/) {
	raise_unsupported("'break' statements are not supported");
}

void TypeChecker::visit(const Continue& /*node*/) {
	raise_unsupported("'continue' statements are not supported");
}

void TypeChecker::visit(const Assume& assume) {
	assert(assume.expr);
	if (assume.expr->sort() == Sort::PTR) {
		auto flat = expression_to_flat_binary_expression(*assume.expr);
		assert(flat.rhs_var.has_value() ^ flat.rhs_null.has_value());
		if (flat.rhs_var.has_value()) {
			this->check_assume_pointer(assume, *flat.lhs, flat.op, **flat.rhs_var);
		} else {
			this->check_assume_pointer(assume, *flat.lhs, flat.op, **flat.rhs_null);
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
		assert(flat.rhs_var.has_value() ^ flat.rhs_null.has_value());
		if (flat.rhs_var.has_value()) {
			this->check_assert_pointer(*this->current_assert, *flat.lhs, flat.op, **flat.rhs_var);
		} else {
			this->check_assert_pointer(*this->current_assert, *flat.lhs, flat.op, **flat.rhs_null);
		}
	
	} else {
		raise_unsupported("assertions with data expressions are not supported");
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

void TypeChecker::visit(const Assignment& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Assignment&)");
}

void TypeChecker::visit(const Enter& enter) {
	std::vector<std::reference_wrapper<const cola::VariableDeclaration>> params;
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
	raise_unsupported("calls to 'MACRO' functions are not supported");
}

void TypeChecker::visit(const CompareAndSwap& /*node*/) {
	raise_unsupported("CAS statements are not supported");
}

void TypeChecker::visit(const Function& function) {
	switch (function.kind) {
		case Function::Kind::INTERFACE:
			this->check_interface_function(function);
			return;

		case Function::Kind::MACRO:
			raise_unsupported("unsupported function kind 'MACRO'"); // TODO: support this or assume inlineing?
			return;

		case Function::Kind::SMR:
			return; // has no implementation (no type check)
	}
}

void TypeChecker::visit(const Program& program) {
	this->check_program(program);
}
