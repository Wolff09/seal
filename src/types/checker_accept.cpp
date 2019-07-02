#include "types/checker.hpp"
#include "types/error.hpp"
#include "cola/util.hpp"

using namespace cola;
using namespace prtypes;


struct ContainsPointerVisitor final : public Visitor {
	bool result = false;
	void visit(const VariableDeclaration& decl) override {
		if (decl.type.sort == Sort::PTR) {
			result = true;
		}
	}
	void visit(const NullValue& /*node*/) override { result = true; }
	void visit(const Dereference& /*node*/) override { result = true; }
	void visit(const BooleanValue& /*node*/) override { /* do nothing */ }
	void visit(const EmptyValue& /*node*/) override { /* do nothing */ }
	void visit(const MaxValue& /*node*/) override { /* do nothing */ }
	void visit(const MinValue& /*node*/) override { /* do nothing */ }
	void visit(const NDetValue& /*node*/) override { /* do nothing */ }
	void visit(const VariableExpression& expr) override { expr.decl.accept(*this); }
	void visit(const NegatedExpression& expr) override { expr.expr->accept(*this); }
	void visit(const BinaryExpression& expr) override {
		expr.lhs->accept(*this);
		expr.rhs->accept(*this);
	}

	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Expression&)"); }
	void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const InvariantActive&)"); }
	void visit(const Sequence& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Sequence&)"); }
	void visit(const Scope& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Scope&)"); }
	void visit(const Atomic& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Atomic&)"); }
	void visit(const Choice& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Choice&)"); }
	void visit(const IfThenElse& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const IfThenElse&)"); }
	void visit(const Loop& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Loop&)"); }
	void visit(const While& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const While&)"); }
	void visit(const Skip& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Skip&)"); }
	void visit(const Break& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Break&)"); }
	void visit(const Continue& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Continue&)"); }
	void visit(const Assume& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Assume&)"); }
	void visit(const Assert& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Assert&)"); }
	void visit(const AngelChoose& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const AngelChoose&)"); }
	void visit(const AngelActive& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const AngelActive&)"); }
	void visit(const AngelContains& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const AngelContains&)"); }
	void visit(const Return& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Return&)"); }
	void visit(const Malloc& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Malloc&)"); }
	void visit(const Assignment& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Assignment&)"); }
	void visit(const Enter& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Enter&)"); }
	void visit(const Exit& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Exit&)"); }
	void visit(const Macro& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Macro&)"); }
	void visit(const CompareAndSwap& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const CompareAndSwap&)"); }
	void visit(const Function& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) override { throw std::logic_error("Unexpected invocation: ContainsPointerVisitor::visit(const Program&)"); }
};

bool contains_pointer(const Expression& expr) {
	ContainsPointerVisitor visitor;
	expr.accept(visitor);
	return visitor.result;
}

struct IsBinaryExpressionVisitor final : public Visitor {
	bool result = false;
	void visit(const BinaryExpression& /*node*/) override { result = true; }
	void visit(const VariableDeclaration& /*node*/) override { result = false; }
	void visit(const Expression& /*node*/) override { result = false; }
	void visit(const BooleanValue& /*node*/) override { result = false; }
	void visit(const NullValue& /*node*/) override { result = false; }
	void visit(const EmptyValue& /*node*/) override { result = false; }
	void visit(const MaxValue& /*node*/) override { result = false; }
	void visit(const MinValue& /*node*/) override { result = false; }
	void visit(const NDetValue& /*node*/) override { result = false; }
	void visit(const VariableExpression& /*node*/) override { result = false; }
	void visit(const NegatedExpression& /*node*/) override { result = false; }
	void visit(const Dereference& /*node*/) override { result = false; }
	void visit(const InvariantExpression& /*node*/) override { result = false; }
	void visit(const InvariantActive& /*node*/) override { result = false; }
	void visit(const Sequence& /*node*/) override { result = false; }
	void visit(const Scope& /*node*/) override { result = false; }
	void visit(const Atomic& /*node*/) override { result = false; }
	void visit(const Choice& /*node*/) override { result = false; }
	void visit(const IfThenElse& /*node*/) override { result = false; }
	void visit(const Loop& /*node*/) override { result = false; }
	void visit(const While& /*node*/) override { result = false; }
	void visit(const Skip& /*node*/) override { result = false; }
	void visit(const Break& /*node*/) override { result = false; }
	void visit(const Continue& /*node*/) override { result = false; }
	void visit(const Assume& /*node*/) override { result = false; }
	void visit(const Assert& /*node*/) override { result = false; }
	void visit(const AngelChoose& /*node*/) override { result = false; }
	void visit(const AngelActive& /*node*/) override { result = false; }
	void visit(const AngelContains& /*node*/) override { result = false; }
	void visit(const Return& /*node*/) override { result = false; }
	void visit(const Malloc& /*node*/) override { result = false; }
	void visit(const Assignment& /*node*/) override { result = false; }
	void visit(const Enter& /*node*/) override { result = false; }
	void visit(const Exit& /*node*/) override { result = false; }
	void visit(const Macro& /*node*/) override { result = false; }
	void visit(const CompareAndSwap& /*node*/) override { result = false; }
	void visit(const Function& /*node*/) override { result = false; }
	void visit(const Program& /*node*/) override { result = false; }
};

bool is_binary_expression(const Expression& expression) {
	IsBinaryExpressionVisitor visitor;
	expression.accept(visitor);
	return visitor.result;
}

struct TypeCheckBaseVisitor : public Visitor {
	virtual void visit(const VariableDeclaration& /*node*/) override { /* do nothing */ }
	virtual void visit(const Expression& /*node*/) override { /* do nothing */ }
	virtual void visit(const BooleanValue& /*node*/) override { /* do nothing */ }
	virtual void visit(const NullValue& /*node*/) override { /* do nothing */ }
	virtual void visit(const EmptyValue& /*node*/) override { /* do nothing */ }
	virtual void visit(const MaxValue& /*node*/) override { /* do nothing */ }
	virtual void visit(const MinValue& /*node*/) override { /* do nothing */ }
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
	virtual void visit(const AngelChoose& /*node*/) override { /* do nothing */ }
	virtual void visit(const AngelActive& /*node*/) override { /* do nothing */ }
	virtual void visit(const AngelContains& /*node*/) override { /* do nothing */ }
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
	void visit(const VariableExpression& var) override {
		this->decl = &var.decl;
	}
};

struct VariableExpressionOrDereferenceOrNullOrValueVisitor final : public TypeCheckBaseVisitor {
	const VariableDeclaration* decl = nullptr;
	const Dereference* deref = nullptr;
	const NullValue* null = nullptr;
	const Expression* value = nullptr;
	void visit(const VariableExpression& var) override { this->decl = &var.decl; }
	void visit(const NullValue& null) override { this->null = &null; }
	void visit(const Dereference& deref) override { this->deref = &deref; }
	void visit(const BooleanValue& value) override { this->value = &value; }
	void visit(const EmptyValue& value) override { this->value = &value; }
	void visit(const MaxValue& value) override { this->value = &value; }
	void visit(const MinValue& value) override { this->value = &value; }
	void visit(const NDetValue& value) override { this->value = &value; }
};

struct BinaryExpressionVisitor final : public TypeCheckBaseVisitor {
	const BinaryExpression* binary;
	void visit(const BinaryExpression& expr) override { this->binary = &expr; }
};


const VariableDeclaration& TypeChecker::expression_to_variable(const Expression& expression) {
	VariableExpressionVisitor visitor;
	expression.accept(visitor);
	if (!visitor.decl) {
		std::cout << "FAILING WITH EXPR: ";
		cola::print(expression, std::cout);
	}
	conditionally_raise_error<UnsupportedConstructError>(!visitor.decl, "unsupported expression; expected a variable");
	assert(visitor.decl);
	return *visitor.decl;
}

TypeChecker::VariableOrDereferenceOrNull TypeChecker::expression_to_variable_or_dereference_or_null_or_value(const Expression& expression) {
	VariableExpressionOrDereferenceOrNullOrValueVisitor visitor;
	expression.accept(visitor);
	// conditionally_raise_error<UnsupportedConstructError>(!visitor.decl && !visitor.deref && !visitor.null, "unsupported expression; expected a variable, a dereference, or 'NULL'");
	// assert(!!visitor.decl ^ !!visitor.deref ^ !!visitor.null);
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
	if (visitor.value) {
		result.value = visitor.value;
	}
	return result;
}

TypeChecker::FlatBinaryExpression TypeChecker::expression_to_flat_binary_expression(const Expression& expression) {
	conditionally_raise_error<UnsupportedConstructError>(!is_binary_expression(expression), "unsupported expression; expected a binary expression");
	BinaryExpressionVisitor visitor;
	expression.accept(visitor);
	conditionally_raise_error<UnsupportedConstructError>(!visitor.binary, "unsupported expression; expected a binary expression");
	assert(visitor.binary);
	assert(visitor.binary->lhs);
	assert(visitor.binary->rhs);
	TypeChecker::FlatBinaryExpression result;
	// result.lhs = &this->expression_to_variable(*visitor.binary->lhs);
	result.lhs = this->expression_to_variable_or_dereference_or_null_or_value(*visitor.binary->lhs);
	result.op = visitor.binary->op;
	result.rhs = this->expression_to_variable_or_dereference_or_null_or_value(*visitor.binary->rhs);
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

void TypeChecker::visit(const MaxValue& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const MaxValue&)");
}

void TypeChecker::visit(const MinValue& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const MinValue&)");
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
	this->check_annotated_statement(atomic);
	bool previously_inside_atomic = this->inside_atomic;
	if (!previously_inside_atomic) {
		this->check_atomic_begin();
	}
	inside_atomic = true;
	assert(atomic.body);
	atomic.body->accept(*this);
	this->inside_atomic = previously_inside_atomic;
	if (!previously_inside_atomic) {
		this->check_atomic_end();
	}
}

void TypeChecker::visit(const Choice& choice) {
	this->check_choice(choice);
}

void TypeChecker::visit(const IfThenElse& ite) {
	this->check_annotated_statement(ite);
	this->check_ite(ite);
}

void TypeChecker::visit(const Loop& loop) {
	this->check_loop(loop);
}

void TypeChecker::visit(const While& whl) {
	this->check_annotated_statement(whl);
	this->check_while(whl);
}

void TypeChecker::visit_command_begin() {
	if (!this->inside_atomic) {
		this->check_atomic_begin();
	}
}

void TypeChecker::visit_command_end() {
	if (!this->inside_atomic) {
		this->check_atomic_end();
	}
}

void TypeChecker::visit(const Skip& skip) {
	this->visit_command_begin();
	this->check_skip(skip);
	this->visit_command_end();
}

void TypeChecker::visit(const Break& brk) {
	this->visit_command_begin();
	this->check_break(brk);
	// raise_error<UnsupportedConstructError>("'break' statements are not supported");
	this->visit_command_end();
}

void TypeChecker::visit(const Continue& /*node*/) {
	this->visit_command_begin();
	raise_error<UnsupportedConstructError>("'continue' statements are not supported");
	this->visit_command_end();
}

void TypeChecker::visit(const Assume& assume) {
	this->visit_command_begin();
	assert(assume.expr);
	if (!contains_pointer(*assume.expr)) {
		this->check_assume_nonpointer(assume, *assume.expr);

	} else {
		auto flat = expression_to_flat_binary_expression(*assume.expr);
		conditionally_raise_error<UnsupportedConstructError>(!(flat.lhs.var.has_value() || flat.lhs.deref.has_value()), "unrecognized left-hand-side of condition in 'assume'; was expecting a variable or a dereference");
		assert(flat.lhs.var.has_value() ^ flat.lhs.deref.has_value());
		if (flat.lhs.var.has_value()) {
			if ((*flat.lhs.var)->type.sort == Sort::PTR) {
				if (flat.rhs.deref.has_value()) {
					auto& deref_var = expression_to_variable(*(*flat.rhs.deref)->expr);
					this->check_assume_pointer(assume, **flat.lhs.var, flat.op, **flat.rhs.deref, deref_var);
				} else {
					conditionally_raise_error<UnsupportedConstructError>(!(flat.rhs.var.has_value() ^ flat.rhs.null.has_value()), "unrecognized right-hand-side of condition in 'assume'; was expecting a variable or 'NULL'");
					// assert(flat.rhs.var.has_value() ^ flat.rhs.null.has_value());
					if (flat.rhs.var.has_value()) {
						this->check_assume_pointer(assume, **flat.lhs.var, flat.op, **flat.rhs.var);
					} else {
						this->check_assume_pointer(assume, **flat.lhs.var, flat.op, **flat.rhs.null);
					}	
				}

			} else {
				assert(flat.rhs.deref.has_value());
				assert(false);
				// this->check_assume_nonpointer(assume, *assume.expr);
			}
		} else if (flat.lhs.deref.has_value()) {
			conditionally_raise_error<UnsupportedConstructError>((*flat.lhs.deref)->sort() == Sort::PTR, "unsupported left-hand-side of condition in 'assume'; dereference must not evaluate to sort 'PTR'");
			conditionally_raise_error<UnsupportedConstructError>(!flat.rhs.value.has_value(), "unsupported right-hand-side of condition in 'assume'; dereference must be compared to a value");

			auto& deref_var = expression_to_variable(*(*flat.lhs.deref)->expr);
			this->check_assume_pointer(assume, **flat.lhs.deref, deref_var, flat.op, **flat.rhs.value);
		} else {
			assert(false);
		}
	}
	this->visit_command_end();
}

void TypeChecker::visit(const Assert& assrt) {
	this->visit_command_begin();
	this->current_assert = &assrt;
	assert(assrt.inv);
	assrt.inv->accept(*this);
	this->current_assert = nullptr;
	this->visit_command_end();
}

void TypeChecker::visit(const AngelChoose& /*angel*/) {
	this->check_angel_choose();
}

void TypeChecker::visit(const AngelActive& /*angel*/) {
	this->check_angel_active();
}

void TypeChecker::visit(const AngelContains& angel) {
	this->check_angel_contains(angel.var);
}

void TypeChecker::visit(const InvariantExpression& invariant) {
	assert(invariant.expr);
	if (!contains_pointer(*invariant.expr)) {
		this->check_assert_nonpointer(*this->current_assert, *invariant.expr);
	} else {
		auto flat = expression_to_flat_binary_expression(*invariant.expr);
		conditionally_raise_error<UnsupportedConstructError>(!flat.rhs.var.has_value(), "unrecognized left-hand-side of condition in 'assert'; was expecting a variable");
		if ((*flat.lhs.var)->type.sort == Sort::PTR) {
			conditionally_raise_error<UnsupportedConstructError>(flat.rhs.deref.has_value(), "dereferences within conditions are not supported");
			conditionally_raise_error<UnsupportedConstructError>(!(flat.rhs.var.has_value() ^ flat.rhs.null.has_value()), "unrecognized right-hand-side of condition in 'assert'; was expecting a variable or 'NULL'");
			// assert(flat.rhs.var.has_value() ^ flat.rhs.null.has_value());
			if (flat.rhs.var.has_value()) {
				this->check_assert_pointer(*this->current_assert, **flat.lhs.var, flat.op, **flat.rhs.var);
			} else {
				this->check_assert_pointer(*this->current_assert, **flat.lhs.var, flat.op, **flat.rhs.null);
			}
		
		} else {
			this->check_assert_nonpointer(*this->current_assert, *invariant.expr);
		}
	}
}

void TypeChecker::visit(const InvariantActive& invariant) {
	assert(invariant.expr);
	auto expr = expression_to_variable_or_dereference_or_null_or_value(*invariant.expr);
	conditionally_raise_error<UnsupportedConstructError>(!(expr.var.has_value() || expr.deref.has_value()), "unsupported 'active' expression; expected a variable or a dereference");
	assert(expr.var.has_value() ^ expr.deref.has_value());
	if (expr.var.has_value()) {
		this->check_assert_active(*this->current_assert, **expr.var);
	} else if (expr.deref.has_value()) {
		auto& var = expression_to_variable(*(*expr.deref)->expr);
		this->check_assert_active(*this->current_assert, **expr.deref, var);
	} else {
		assert(false);
	}
}

void TypeChecker::visit(const Return& retrn) {
	this->visit_command_begin();
	assert(retrn.expr);
	this->check_return(retrn, expression_to_variable(*retrn.expr));
	this->visit_command_end();
}

void TypeChecker::visit(const Malloc& malloc) {
	this->visit_command_begin();
	this->check_malloc(malloc, malloc.lhs);
	this->visit_command_end();
}

void TypeChecker::visit(const Assignment& assignment) {
	this->visit_command_begin();
	assert(assignment.lhs);
	assert(assignment.rhs);
	assert(assignment.lhs->type().sort == assignment.rhs->type().sort);

	if (assignment.lhs->type().sort == Sort::PTR) {
		auto lhs = expression_to_variable_or_dereference_or_null_or_value(*assignment.lhs);
		auto rhs = expression_to_variable_or_dereference_or_null_or_value(*assignment.rhs);

		conditionally_raise_error<UnsupportedConstructError>(lhs.null.has_value(), "cannot assign to 'NULL'");
		conditionally_raise_error<UnsupportedConstructError>(lhs.deref.has_value() && rhs.deref.has_value(), "simultanious dereference on left-hand-side and right-hand-side of assignment are not supported");
		assert(lhs.var.has_value() ^ lhs.deref.has_value());
		// assert(rhs.var.has_value() ^ rhs.deref.has_value() ^ rhs.null.has_value());
		conditionally_raise_error<UnsupportedConstructError>(!(lhs.var.has_value() ^ lhs.deref.has_value()), "unrecognized left-hand-side of assignment; expected a variable or a dereference");
		conditionally_raise_error<UnsupportedConstructError>(!(rhs.var.has_value() ^ rhs.deref.has_value() ^ rhs.null.has_value()), "unrecognized right-hand-side of assignment; expected a variable, a dereference, or 'NULL'");

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
	} else {
		auto lhs = expression_to_variable_or_dereference_or_null_or_value(*assignment.lhs);
		auto rhs = expression_to_variable_or_dereference_or_null_or_value(*assignment.rhs);

		if (lhs.deref.has_value()) {
			auto& deref_var = expression_to_variable(*(*lhs.deref)->expr);
			conditionally_raise_error<UnsupportedConstructError>(!(rhs.var.has_value() || rhs.value.has_value()), "unrecognized right-hand-side; data selector must be assigned from variable");
			check_assign_nonpointer(assignment, **lhs.deref, deref_var, **rhs.var);

		} else if (rhs.deref.has_value()) {
			auto& deref_var = expression_to_variable(*(*rhs.deref)->expr);
			conditionally_raise_error<UnsupportedConstructError>(!lhs.var.has_value(), "unrecognized left-hand-side; data selector must be read into variable");
			check_assign_nonpointer(assignment, **lhs.var, **rhs.deref, deref_var);

		} else {
			conditionally_raise_error<UnsupportedConstructError>(contains_pointer(*assignment.lhs), "unrecognized left-hand-side; data expression must not contain pointers");
			conditionally_raise_error<UnsupportedConstructError>(contains_pointer(*assignment.rhs), "unrecognized right-hand-side; data expression must not contain pointers");
			check_assign_nonpointer(assignment, *assignment.lhs, *assignment.rhs);
		}
	}
	this->visit_command_end();
}

void TypeChecker::visit(const Enter& enter) {
	this->visit_command_begin();
	std::vector<std::reference_wrapper<const VariableDeclaration>> params;
	for (const auto& arg : enter.args) {
		assert(arg);
		params.push_back(expression_to_variable(*arg));
	}

	this->check_enter(enter, std::move(params));
	this->visit_command_end();
}

void TypeChecker::visit(const Exit& exit) {
	this->visit_command_begin();
	this->check_exit(exit);
	this->visit_command_end();
}

void TypeChecker::visit(const Macro& /*node*/) {
	this->visit_command_begin();
	raise_error<UnsupportedConstructError>("calls to 'MACRO' functions are not supported");
	this->visit_command_end();
}

void TypeChecker::visit(const CompareAndSwap& /*node*/) {
	this->visit_command_begin();
	raise_error<UnsupportedConstructError>("CAS statements are not supported");
	this->visit_command_end();
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
