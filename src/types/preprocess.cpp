#include "types/preprocess.hpp"
#include "cola/util.hpp"
#include "cola/transform.hpp"
#include "types/error.hpp"
#include <iostream>

using namespace cola;
using namespace prtypes;


struct PreprocessingVisitor final : public NonConstVisitor {
	bool in_atomic = false;
	bool found_return = false;
	bool found_cmd = false;

	void visit(VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(VariableDeclaration&)"); }
	void visit(Expression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(Expression&)"); }
	void visit(BooleanValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(BooleanValue&)"); }
	void visit(NullValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(NullValue&)"); }
	void visit(EmptyValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(EmptyValue&)"); }
	void visit(NDetValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(NDetValue&)"); }
	void visit(VariableExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(VariableExpression&)"); }
	void visit(NegatedExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(NegatedExpression&)"); }
	void visit(BinaryExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(BinaryExpression&)"); }
	void visit(Dereference& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(Dereference&)"); }
	void visit(InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(InvariantExpression&)"); }
	void visit(InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(InvariantActive&)"); }
	
	void visit(IfThenElse& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(IfThenElse&)"); }
	void visit(While& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(While&)"); }
	void visit(CompareAndSwap& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(CompareAndSwap&)"); }
	void visit(Break& /*node*/) { raise_error<UnsupportedConstructError>("'break' not supported"); }
	void visit(Continue& /*node*/) { raise_error<UnsupportedConstructError>("'continue' not supported"); }
	void visit(Macro& /*node*/) { raise_error<UnsupportedConstructError>("'continue' not supported"); }

	void handle_statement(std::unique_ptr<Statement>& stmt) {
		found_cmd = false;
		stmt->accept(*this);
		if (found_cmd && !in_atomic) {
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

	void visit(Skip& /*node*/) { found_cmd = true; }
	void visit(Assume& /*node*/) { found_cmd = true; }
	void visit(Assert& /*node*/) { found_cmd = true; }
	void visit(Return& /*node*/) { found_cmd = true; found_return = true; }
	void visit(Malloc& /*node*/) { found_cmd = true; }
	void visit(Assignment& /*node*/) { found_cmd = true; }
	void visit(Enter& /*node*/) { found_cmd = true; }
	void visit(Exit& /*node*/) { found_cmd = true; }

	void visit(Function& node) {
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

void prtypes::preprocess(Program& program) {
	// // cola::remove_cas(program); // TODO: activate
	cola::remove_scoped_variables(program);
	cola::remove_conditionals(program);
	cola::remove_useless_scopes(program);

	PreprocessingVisitor visitor;
	program.accept(visitor);
}