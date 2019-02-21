#include "types/rmraces.hpp"
#include "types/cave.hpp"
#include "cola/util.hpp"
#include <iostream>

using namespace cola;
using namespace prtypes;


struct LocalExpressionVisitor final : public Visitor {
	bool result = true;
	void visit(const VariableDeclaration& decl) {
		if (decl.is_shared) {
			result = false;
		}
	}
	void visit(const Dereference& /*node*/) {
		// memory access may not be local
		result = false;
	}
	void visit(const BooleanValue& /*node*/) { /* do nothing */ }
	void visit(const NullValue& /*node*/) { /* do nothing */ }
	void visit(const EmptyValue& /*node*/) { /* do nothing */ }
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

bool is_expression_local(const Expression& expr) {
	LocalExpressionVisitor visitor;
	expr.accept(visitor);
	return visitor.result;
}

struct AssertionInsertionVisitor : public NonConstVisitor {
	const Command& to_find;
	std::unique_ptr<Command> to_insert;
	Statement* insertion_parent = nullptr; // may come in handy to undo insertion
	AssertionInsertionVisitor(const Command& to_find, std::unique_ptr<Command> to_insert) : to_find(to_find), to_insert(std::move(to_insert)) {}

	void visit(VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(VariableDeclaration&)"); }
	void visit(Expression& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(Expression&)"); }
	void visit(BooleanValue& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(BooleanValue&)"); }
	void visit(NullValue& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(NullValue&)"); }
	void visit(EmptyValue& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(EmptyValue&)"); }
	void visit(NDetValue& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(NDetValue&)"); }
	void visit(VariableExpression& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(VariableExpression&)"); }
	void visit(NegatedExpression& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(NegatedExpression&)"); }
	void visit(BinaryExpression& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(BinaryExpression&)"); }
	void visit(Dereference& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(Dereference&)"); }
	void visit(InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(InvariantExpression&)"); }
	void visit(InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(InvariantActive&)"); }
	
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

	std::unique_ptr<Sequence> mk_insertion(std::unique_ptr<Statement> found) {
		assert(found.get() == &to_find);
		assert(this->to_insert);
		assert(!this->insertion_parent);
		auto result = std::make_unique<Sequence>(std::move(this->to_insert), std::move(found));
		this->insertion_parent = result.get();
		assert(!this->to_insert);
		assert(this->insertion_parent);
		return result;
	}
	void visit(Sequence& seq) {
		// handle first
		assert(seq.first);
		if (seq.first.get() == &this->to_find) {
			seq.first = mk_insertion(std::move(seq.first));
		} else {
			seq.first->accept(*this);
		}
		// handle second
		assert(seq.second);
		if (seq.second.get() == &this->to_find) {
			seq.second = mk_insertion(std::move(seq.second));
		} else {
			seq.second->accept(*this);
		}
	}
	void visit(Scope& scope) {
		assert(scope.body);
		if (scope.body.get() == &this->to_find) {
			scope.body = mk_insertion(std::move(scope.body));
		} else {
			scope.body->accept(*this);
		}
	}
	void visit(Atomic& node) {
		assert(node.body);
		node.body->accept(*this);
	}
	void visit(Choice& node) {
		for (const auto& branch : node.branches) {
			assert(branch);
			branch->accept(*this);
		}
	}
	void visit(IfThenElse& node) {
		assert(node.ifBranch);
		assert(node.elseBranch);
		node.ifBranch->accept(*this);
		node.elseBranch->accept(*this);
	}
	void visit(Loop& node) {
		assert(node.body);
		node.body->accept(*this);
	}
	void visit(While& node) {
		assert(node.body);
		node.body->accept(*this);
	}
	void visit(Function& function) {
		if (function.kind == Function::SMR) {
			return;
		}
		assert(function.body);
		function.body->accept(*this);
	}
	void visit(Program& program) {
		assert(program.initializer);
		program.initializer->accept(*this);
		for (const auto& function : program.functions) {
			assert(function);
			function->accept(*this);
		}
	}
};

void insert_active_assertion(Program& program, const GuaranteeTable& guarantee_table, const Command& cmd, const VariableDeclaration& var) {
	// TODO: we may need to add the assertion earlier than the cmd that fails (when reading in var, it may be active, but when accessing it may no longer be active)

	// make insertion to insert
	auto assertion = std::make_unique<Assert>(std::make_unique<InvariantActive>(std::make_unique<VariableExpression>(var)));
	const Assert& inserted_assertion = *assertion.get();

	// patch program
	AssertionInsertionVisitor visitor(cmd, std::move(assertion));
	program.accept(visitor);
	assert(!visitor.to_insert);

	// check added assertion for validity
	bool is_inserted_assertion_valid = prtypes::discharge_assertions(program, guarantee_table, { inserted_assertion });
	if (!is_inserted_assertion_valid) {
		raise_error<RefinementError>("could not infer valid assertion to fix pointer race");
	}
}

void prtypes::try_fix_pointer_race(Program& program, const GuaranteeTable& guarantee_table, const UnsafeAssumeError& error) {
	// TODO: local moverness
	// try to move the offending command
	assert(error.pc.expr);
	if (is_expression_local(*error.pc.expr)) {
		// TODO: check if can move; if so, move and return
		// TODO: find a list of previous assume/assert statements, try to add an assert(active(...)) there, if possible then do the check there and store the result in a variable (also propagate this check to other assertions)
		// throw std::logic_error("not yet implemented: prtypes::try_fix_pointer_race (local moverness)");
		std::cout << "(Ignoring the fact that the assume statement is purely local" << std::endl;
	}

	// insert assertion for offending command
	insert_active_assertion(program, guarantee_table, error.pc, error.var);
}

void prtypes::try_fix_pointer_race(cola::Program& program, const GuaranteeTable& guarantee_table, const UnsafeCallError& error) {
	if (&error.pc.decl == &guarantee_table.observer_store.retire_function) {
		assert(error.pc.args.size() == 1);
		assert(error.pc.args.at(0));
		const VariableExpression& expr = *static_cast<const VariableExpression*>(error.pc.args.at(0).get()); // TODO: unhack this
		insert_active_assertion(program, guarantee_table, error.pc, expr.decl);

	} else {
		raise_error<RefinementError>("cannot recover from unsafe call to function " + error.pc.decl.name);
	}
}

void prtypes::try_fix_pointer_race(cola::Program& program, const GuaranteeTable& guarantee_table, const UnsafeDereferenceError& error) {
	insert_active_assertion(program, guarantee_table, error.pc, error.var);
	// std::cout << "in: ";
	// cola::print(error.pc, std::cout);
	// std::cout << std::endl;
	// std::cout << std::endl;
	// cola::print(program, std::cout);
	// std::cout << std::endl;
	// throw std::logic_error("not yet implemented: try_fix_pointer_race for UnsafeDereferenceError");
}







