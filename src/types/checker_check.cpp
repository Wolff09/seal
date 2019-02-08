#include "types/check.hpp"
#include "types/inference.hpp"
#include "types/error.hpp"
#include "types/util.hpp"

using namespace cola;
using namespace prtypes;


void TypeChecker::ensure_valid(const VariableDeclaration& variable) {
	assert(prtypes::has_binding(current_type_environment, variable));
	bool is_valid = entails_valid(current_type_environment.at(variable));
	raise_type_error_if(!is_valid, "invalid pointer used");
}


void TypeChecker::check_skip(const Skip& /*skip*/) {
	// do nothing
}

void TypeChecker::check_malloc(const Malloc& /*malloc*/, const VariableDeclaration& ptr) {
	raise_type_error_if(ptr.is_shared, "allocations must not target shared variables");
	assert(prtypes::has_binding(current_type_environment, ptr));
	current_type_environment.at(ptr).clear();
	current_type_environment.at(ptr).insert(guarantee_table.local_guarantee());
}

void TypeChecker::check_enter(const Enter& /*enter*/, std::vector<std::reference_wrapper<VariableDeclaration>> /*params*/) {
	throw std::logic_error("not yet implemented: TypeChecker::check_enter(const Enter& enter, std::vector<std::reference_wrapper<VariableDeclaration>> params)");
}

void TypeChecker::check_exit(const Exit& /*exit*/) {
	throw std::logic_error("not yet implemented: TypeChecker::check_exit(const Exit& exit)");
}

void TypeChecker::check_assume_nonpointer(const Assume& /*assume*/, const Expression& /*expr*/) {
	// do nothing
}

void TypeChecker::check_assume_pointer(const Assume& /*assume*/, const VariableDeclaration& lhs, BinaryExpression::Operator op, const VariableDeclaration& rhs) {
	if (op == BinaryExpression::Operator::EQ) {
		assert(prtypes::has_binding(current_type_environment, lhs));
		assert(prtypes::has_binding(current_type_environment, rhs));
		
		ensure_valid(lhs);
		ensure_valid(rhs);
		
		GuaranteeSet sum = prtypes::merge(current_type_environment.at(lhs), current_type_environment.at(rhs));
		sum.erase(guarantee_table.local_guarantee());
		current_type_environment.at(lhs) = sum;
		current_type_environment.at(rhs) = sum;

	} else {
		// do nothing
	}
}

void TypeChecker::check_assume_pointer(const Assume& /*assume*/, const VariableDeclaration& lhs, BinaryExpression::Operator op, const NullValue& /*rhs*/) {
	if (op == BinaryExpression::Operator::EQ) {
		// NULL is always valid
		assert(prtypes::has_binding(current_type_environment, lhs));
		ensure_valid(lhs);
		current_type_environment.at(lhs).erase(guarantee_table.local_guarantee());

	} else {
		// do nothing
	}
}

void TypeChecker::check_assert_pointer(const Assert& /*assert*/, const VariableDeclaration& lhs, BinaryExpression::Operator op, const VariableDeclaration& rhs) {
	if (op == BinaryExpression::Operator::EQ) {
		assert(prtypes::has_binding(current_type_environment, lhs));
		assert(prtypes::has_binding(current_type_environment, rhs));
		
		GuaranteeSet sum = prtypes::merge(current_type_environment.at(lhs), current_type_environment.at(rhs));
		sum.erase(guarantee_table.local_guarantee()); // TODO: correct?
		current_type_environment.at(lhs) = sum;
		current_type_environment.at(rhs) = sum;

	} else {
		raise_unsupported("unsupported comparison operator in assertions; must be '=='");
	}
}

void TypeChecker::check_assert_pointer(const Assert& /*assert*/, const VariableDeclaration& lhs, BinaryExpression::Operator op, const NullValue& /*rhs*/) {
	if (op == BinaryExpression::Operator::EQ) {
		assert(prtypes::has_binding(current_type_environment, lhs));
		current_type_environment.at(lhs).erase(guarantee_table.local_guarantee());

	} else {
		raise_unsupported("unsupported comparison operator in assertions; must be '=='");
	}
}

void TypeChecker::check_assert_active(const Assert& /*assert*/, const VariableDeclaration& ptr) {
	assert(prtypes::has_binding(current_type_environment, ptr));
	current_type_environment.at(ptr).insert(guarantee_table.active_guarantee());
}

void TypeChecker::check_assign_pointer(const Assignment& /*node*/, const VariableDeclaration& lhs, const VariableDeclaration& rhs) {
	assert(prtypes::has_binding(current_type_environment, lhs));
	assert(prtypes::has_binding(current_type_environment, rhs));

	GuaranteeSet result(current_type_environment.at(lhs));
	result.erase(guarantee_table.local_guarantee());

	current_type_environment.at(lhs) = result;
	current_type_environment.at(rhs) = result;
}

void TypeChecker::check_assign_pointer(const Assignment& /*node*/, const VariableDeclaration& lhs, const NullValue& /*rhs*/) {
	assert(prtypes::has_binding(current_type_environment, lhs));
	current_type_environment.at(lhs).clear();
}

void TypeChecker::check_assign_pointer(const Assignment& /*node*/, const Dereference& /*lhs_deref*/, const VariableDeclaration& lhs_var, const VariableDeclaration& rhs) {
	assert(prtypes::has_binding(current_type_environment, lhs_var));
	assert(prtypes::has_binding(current_type_environment, rhs));

	ensure_valid(lhs_var);
	current_type_environment.at(rhs).erase(guarantee_table.local_guarantee());
}

void TypeChecker::check_assign_pointer(const Assignment& /*node*/, const Dereference& /*lhs_deref*/, const VariableDeclaration& /*lhs_var*/, const NullValue& /*rhs*/) {
	// do nothing
}

void TypeChecker::check_assign_pointer(const Assignment& /*node*/, const VariableDeclaration& lhs, const Dereference& /*rhs_deref*/, const VariableDeclaration& rhs_var) {
	assert(prtypes::has_binding(current_type_environment, lhs));
	assert(prtypes::has_binding(current_type_environment, rhs_var));

	ensure_valid(rhs_var);
	current_type_environment.at(lhs).clear();
}

void TypeChecker::check_assign_nonpointer(const Assignment& /*node*/, const Expression& /*lhs*/, const Expression& /*rhs*/) {
	// do nothing
}

void TypeChecker::check_assign_nonpointer(const Assignment& /*node*/, const Dereference& /*lhs_deref*/, const VariableDeclaration& lhs_var, const Expression& /*rhs*/) {
	assert(prtypes::has_binding(current_type_environment, lhs_var));
	ensure_valid(lhs_var);
}

void TypeChecker::check_assign_nonpointer(const Assignment& /*node*/, const Expression& /*lhs*/, const Dereference& /*rhs_deref*/, const VariableDeclaration& rhs_var) {
	assert(prtypes::has_binding(current_type_environment, rhs_var));
	ensure_valid(rhs_var);
}

void TypeChecker::check_scope(const Scope& scope) {
	// populate current_type_environment with empty guarantees for declared pointer variables
	for (const auto& decl : scope.variables) {
		auto insertion = current_type_environment.insert({ *decl, GuaranteeSet() });
		raise_unsupported_if(!insertion.second, "hiding variable declaration of outer scope not supported");
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

void TypeChecker::check_sequence(const Sequence& sequence) {
	assert(sequence.first);
	assert(sequence.second);
	sequence.first->accept(*this);
	sequence.second->accept(*this);
	// widening is applied directly at the commands
}

void TypeChecker::check_atomic(const Atomic& atomic) {
	assert(atomic.body);
	atomic.body->accept(*this);

	// remove transient guarantees from local pointers, remove all guarantees from shared pointers
	for (auto& [decl, guarantees] : current_type_environment) {
		if (decl.get().is_shared) {
			guarantees.clear();
		} else {
			guarantees = prtypes::prune_transient_guarantees(std::move(guarantees));
		}
	}
}

void TypeChecker::check_choice(const Choice& /*choice*/) {
	throw std::logic_error("not yet implemented: TypeChecker::check_choice(const Choice& choice)");
}

void TypeChecker::check_loop(const Loop& /*loop*/) {
	throw std::logic_error("not yet implemented: TypeChecker::check_loop(const Loop& loop)");
}

void TypeChecker::check_interface_function(const Function& function) {
	current_type_environment.clear();
	function.body->accept(*this);
}

void TypeChecker::check_program(const Program& program) {
	for (const auto& function : program.functions) {
		function->accept(*this);
	}
}
