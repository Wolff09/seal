#include "types/check.hpp"

using namespace cola;
using namespace prtypes;


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

void TypeChecker::visit(const InvariantExpression& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const InvariantExpression&)");
}

void TypeChecker::visit(const InvariantActive& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const InvariantActive&)");
}

void TypeChecker::visit(const Sequence& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Sequence&)");
}

void TypeChecker::visit(const Scope& /*node*/) {
	// TODO: prevent variable hiding?
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Scope&)");
}

void TypeChecker::visit(const Atomic& /*node*/) {
	// TODO: prevent nesting of atomic blocks
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Atomic&)");
}

void TypeChecker::visit(const Choice& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Choice&)");
}

void TypeChecker::visit(const IfThenElse& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const IfThenElse&)");
}

void TypeChecker::visit(const Loop& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Loop&)");
}

void TypeChecker::visit(const While& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const While&)");
}

void TypeChecker::visit(const Skip& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Skip&)");
}

void TypeChecker::visit(const Break& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Break&)");
}

void TypeChecker::visit(const Continue& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Continue&)");
}

void TypeChecker::visit(const Assume& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Assume&)");
}

void TypeChecker::visit(const Assert& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Assert&)");
}

void TypeChecker::visit(const Return& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Return&)");
}

void TypeChecker::visit(const Malloc& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Malloc&)");
}

void TypeChecker::visit(const Assignment& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Assignment&)");
}

void TypeChecker::visit(const Enter& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Enter&)");
}

void TypeChecker::visit(const Exit& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Exit&)");
}

void TypeChecker::visit(const Macro& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Macro&)");
}

void TypeChecker::visit(const CompareAndSwap& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const CompareAndSwap&)");
}

void TypeChecker::visit(const Function& /*node*/) {
	// switch (function.kind) {
	// 	case Function::Kind::INTERFACE:
	// 		break;

	// 	case Function::Kind::MACRO:
	// 		raise_unsupported("unsupported function kind 'MACRO'"); // TODO: support this or assume inlineing?
	// 		break;

	// 	case Function::Kind::SMR:
	// 		return; // no type check (has no implementation)
	// }
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Function&)");
}

void TypeChecker::visit(const Program& /*node*/) {
	throw std::logic_error("not yet implemented: TypeChecker::visit(const Program&)");
}
