#include "types/check/check_impl.hpp"
#include "types/inference.hpp"

using namespace cola;
using namespace prtypes;


void remove_transient_guarantees(GuaranteeSet& guarantees) {
	for (auto it = guarantees.cbegin(); it != guarantees.cend(); ) {
		if (it->get().is_transient) {
			it = guarantees.erase(it); // progresses it to the next element
		} else {
			++it;
		}
	}
}


bool TypeCheckStatementVisitor::is_well_typed(const Program& program) {
	program.accept(*this);
	throw std::logic_error("not yet implemented (TypeCheckVisitor::is_well_typed(const Program&)");
}

void TypeCheckStatementVisitor::visit(const Scope& scope) {
	// populate current_type_environment with empty guarantees for declared pointer variables (prevent hiding)
	for (const auto& decl : scope.variables) {
		auto insertion = current_type_environment.insert({ *decl, GuaranteeSet() });
		if (!insertion.second) {
			raise_type_error("hiding variable declaration of outer scope not supported");
		}
	}

	// type check body
	if (scope.body) {
		scope.body->accept(*this);
	}

	// remove added type bindings
	for (const auto& decl : scope.variables) {
		current_type_environment.erase(*decl);
	}
}

void TypeCheckStatementVisitor::visit(const Sequence& sequence) {
	assert(sequence.first);
	assert(sequence.second);
	sequence.first->accept(*this);
	sequence.second->accept(*this);
	// widening is applied directly at the commands
}

void TypeCheckStatementVisitor::visit(const Atomic& atomic) {
	// prevent nesting atomic blocks
	if (in_atomic) {
		raise_type_error("nested 'atomic' blocks are not supported");
	}

	// type check body
	in_atomic = true;
	assert(atomic.body);
	atomic.body->accept(*this);
	in_atomic = false;

	// remove transient guarantees local pointers, remove all guarantees from shared pointers
	for (auto& [decl, guarantees] : current_type_environment) {
		if (decl.get().is_shared) {
			guarantees.clear();
		} else {
			remove_transient_guarantees(guarantees);
		}
	}
}


void TypeCheckStatementVisitor::visit(const Skip& /*node*/) {
	// do nothing
}

void TypeCheckStatementVisitor::visit(const Return& /*node*/) {
	// do nothing
}

void TypeCheckStatementVisitor::visit(const IfThenElse& /*node*/) {
	raise_type_error("unsupported statement 'if'");
}

void TypeCheckStatementVisitor::visit(const While& /*node*/) {
	raise_type_error("unsupported statement 'while'");
}

void TypeCheckStatementVisitor::visit(const Break& /*node*/) {
	raise_type_error("unsupported statement 'break'");
}

void TypeCheckStatementVisitor::visit(const Continue& /*node*/) {
	raise_type_error("unsupported statement 'continue'");
}

void TypeCheckStatementVisitor::visit(const Macro& /*node*/) {
	raise_type_error("macros calls are not supported");
}

void TypeCheckStatementVisitor::visit(const CompareAndSwap& /*node*/) {
	raise_type_error("unsupported statement 'CAS'");
}

void TypeCheckStatementVisitor::visit(const Function& function) {
	switch (function.kind) {
		case Function::Kind::INTERFACE:
			break;

		case Function::Kind::MACRO:
			raise_type_error("unsupported function kind 'MACRO'"); // TODO: support this or assume inlineing?
			break;

		case Function::Kind::SMR:
			return; // no type check (has no implementation)
	}

	current_function = &function;
	current_type_environment.clear();
	function.body->accept(*this);
}

void TypeCheckStatementVisitor::visit(const Program& program) {
	current_program = &program;
	for (const auto& function : program.functions) {
		function->accept(*this);
	}
}

void TypeCheckStatementVisitor::visit(const Malloc& /*node*/) { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Malloc&))"); }
void TypeCheckStatementVisitor::visit(const Assignment& /*node*/) { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Assignment&))"); }
void TypeCheckStatementVisitor::visit(const Choice& /*node*/) { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Choice&))"); }
void TypeCheckStatementVisitor::visit(const Loop& /*node*/) { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Loop&))"); }
void TypeCheckStatementVisitor::visit(const Enter& /*node*/) { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Enter&))"); }
void TypeCheckStatementVisitor::visit(const Exit& /*node*/) { throw std::logic_error("not yet implemented (TypeCheckVisitor::visit(const Exit&))"); }

void TypeCheckStatementVisitor::visit(const VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const VariableDeclaration&)"); }
void TypeCheckStatementVisitor::visit(const Expression& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const Expression&)"); }
void TypeCheckStatementVisitor::visit(const BooleanValue& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const BooleanValue&)"); }
void TypeCheckStatementVisitor::visit(const NullValue& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const NullValue&)"); }
void TypeCheckStatementVisitor::visit(const EmptyValue& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const EmptyValue&)"); }
void TypeCheckStatementVisitor::visit(const NDetValue& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const NDetValue&)"); }
void TypeCheckStatementVisitor::visit(const VariableExpression& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const VariableExpression&)"); }
void TypeCheckStatementVisitor::visit(const NegatedExpression& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const NegatedExpression&)"); }
void TypeCheckStatementVisitor::visit(const BinaryExpression& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const BinaryExpression&)"); }
void TypeCheckStatementVisitor::visit(const Dereference& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const Dereference&)"); }
void TypeCheckStatementVisitor::visit(const InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const InvariantExpression&)"); }
void TypeCheckStatementVisitor::visit(const InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: TypeCheckVisitor::visit(const InvariantActive&)"); }
