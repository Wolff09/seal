#include "types/checker.hpp"
#include "types/inference.hpp"
#include "types/error.hpp"
#include "types/util.hpp"

using namespace cola;
using namespace prtypes;


void TypeChecker::check_annotated_statement(const cola::AnnotatedStatement& /*stmt*/) {
	raise_error<UnsupportedConstructError>("annotations are not supported, use 'assume' statements instead");
}

void TypeChecker::check_command(const cola::Command& command) {
	this->check_annotated_statement(command);
	conditionally_raise_error<UnsupportedConstructError>(!this->inside_atomic, "commands must appear inside an atomic block");
}


bool TypeChecker::is_pointer_valid(const VariableDeclaration& variable) {
	assert(variable.type.sort == Sort::PTR);
	assert(prtypes::has_binding(current_type_environment, variable));
	return entails_valid(current_type_environment.at(variable));
}


void TypeChecker::check_skip(const Skip& /*skip*/) {
	// do nothing
}

void TypeChecker::check_malloc(const Malloc& /*malloc*/, const VariableDeclaration& ptr) {
	conditionally_raise_error<UnsupportedConstructError>(ptr.is_shared, "allocations must not target shared variables");
	assert(prtypes::has_binding(current_type_environment, ptr));
	current_type_environment.at(ptr).clear();
	current_type_environment.at(ptr).insert(guarantee_table.local_guarantee());
}

void TypeChecker::check_enter(const Enter& enter, std::vector<std::reference_wrapper<const VariableDeclaration>> params) {
	// check safe
	SimulationEngine::VariableDeclarationSet invalid;
	for (const auto& variable : params) {
		if (variable.get().type.sort == Sort::PTR && !is_pointer_valid(variable)) {
			invalid.insert(variable);
		}
	}
	bool is_safe_call = guarantee_table.observer_store.simulation.is_safe(enter, params, invalid);
	conditionally_raise_error<UnsafeCallError>(!is_safe_call, enter);

	// update types
	TypeEnv result;
	for (const auto& [decl, guarantees] : this->current_type_environment) {
		result[decl] = inference.infer_enter(guarantees, enter, decl);
	}
	this->current_type_environment = std::move(result);
}

void TypeChecker::check_exit(const Exit& exit) {
	TypeEnv result;
	for (const auto& [decl, guarantees] : this->current_type_environment) {
		result[decl] = inference.infer_exit(guarantees, exit);
	}
	this->current_type_environment = std::move(result);
}


void TypeChecker::check_assume_nonpointer(const Assume& /*assume*/, const Expression& /*expr*/) {
	// do nothing
}

void TypeChecker::check_assume_pointer(const Assume& assume, const VariableDeclaration& lhs, BinaryExpression::Operator op, const VariableDeclaration& rhs) {
	assert(assume.expr);
	if (assume.expr->sort() == Sort::PTR && op == BinaryExpression::Operator::EQ) {
		assert(prtypes::has_binding(current_type_environment, lhs));
		assert(prtypes::has_binding(current_type_environment, rhs));
		
		conditionally_raise_error<UnsafeAssumeError>(is_pointer_valid(lhs), assume, lhs);
		conditionally_raise_error<UnsafeAssumeError>(is_pointer_valid(rhs), assume, rhs);
		
		GuaranteeSet sum = prtypes::merge(current_type_environment.at(lhs), current_type_environment.at(rhs));
		sum.erase(guarantee_table.local_guarantee());
		sum = inference.infer(sum);
		current_type_environment.at(lhs) = sum;
		current_type_environment.at(rhs) = sum;

	} else {
		// do nothing
	}
}

void TypeChecker::check_assume_pointer(const Assume& assume, const VariableDeclaration& lhs, BinaryExpression::Operator op, const NullValue& /*rhs*/) {
	assert(assume.expr);
	if (assume.expr->sort() == Sort::PTR && op == BinaryExpression::Operator::EQ) {
		// NULL is always valid
		assert(prtypes::has_binding(current_type_environment, lhs));
		conditionally_raise_error<UnsafeAssumeError>(is_pointer_valid(lhs), assume, lhs);
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
		sum = inference.infer(sum);
		current_type_environment.at(lhs) = sum;
		current_type_environment.at(rhs) = sum;

	} else {
		raise_error<UnsupportedConstructError>("unsupported comparison operator in assertions; must be '=='");
	}
}

void TypeChecker::check_assert_pointer(const Assert& /*assert*/, const VariableDeclaration& lhs, BinaryExpression::Operator op, const NullValue& /*rhs*/) {
	if (op == BinaryExpression::Operator::EQ) {
		assert(prtypes::has_binding(current_type_environment, lhs));
		current_type_environment.at(lhs).erase(guarantee_table.local_guarantee());

	} else {
		raise_error<UnsupportedConstructError>("unsupported comparison operator in assertions; must be '=='");
	}
}

void TypeChecker::check_assert_active(const Assert& /*assert*/, const VariableDeclaration& ptr) {
	assert(prtypes::has_binding(current_type_environment, ptr));
	GuaranteeSet guarantees = std::move(current_type_environment.at(ptr));
	guarantees.insert(guarantee_table.active_guarantee());
	guarantees = inference.infer(guarantees);
	current_type_environment.at(ptr) = std::move(guarantees);
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

void TypeChecker::check_assign_pointer(const Assignment& assignment, const Dereference& lhs_deref, const VariableDeclaration& lhs_var, const VariableDeclaration& rhs) {
	assert(prtypes::has_binding(current_type_environment, lhs_var));
	assert(prtypes::has_binding(current_type_environment, rhs));

	conditionally_raise_error<UnsafeDereferenceError>(is_pointer_valid(lhs_var), assignment, lhs_deref, lhs_var);
	current_type_environment.at(rhs).erase(guarantee_table.local_guarantee());
}

void TypeChecker::check_assign_pointer(const Assignment& /*node*/, const Dereference& /*lhs_deref*/, const VariableDeclaration& /*lhs_var*/, const NullValue& /*rhs*/) {
	// do nothing
}

void TypeChecker::check_assign_pointer(const Assignment& assignment, const VariableDeclaration& lhs, const Dereference& rhs_deref, const VariableDeclaration& rhs_var) {
	assert(prtypes::has_binding(current_type_environment, lhs));
	assert(prtypes::has_binding(current_type_environment, rhs_var));

	conditionally_raise_error<UnsafeDereferenceError>(is_pointer_valid(rhs_var), assignment, rhs_deref, rhs_var);
	current_type_environment.at(lhs).clear();
}

void TypeChecker::check_assign_nonpointer(const Assignment& /*node*/, const Expression& /*lhs*/, const Expression& /*rhs*/) {
	// do nothing
}

void TypeChecker::check_assign_nonpointer(const Assignment& assignment, const Dereference& lhs_deref, const VariableDeclaration& lhs_var, const Expression& /*rhs*/) {
	assert(prtypes::has_binding(current_type_environment, lhs_var));
	conditionally_raise_error<UnsafeDereferenceError>(is_pointer_valid(lhs_var), assignment, lhs_deref, lhs_var);
}

void TypeChecker::check_assign_nonpointer(const Assignment& assignment, const Expression& /*lhs*/, const Dereference& rhs_deref, const VariableDeclaration& rhs_var) {
	assert(prtypes::has_binding(current_type_environment, rhs_var));
	conditionally_raise_error<UnsafeDereferenceError>(is_pointer_valid(rhs_var), assignment, rhs_deref, rhs_var);
}


void TypeChecker::check_scope(const Scope& scope) {
	// populate current_type_environment with empty guarantees for declared pointer variables
	for (const auto& decl : scope.variables) {
		auto insertion = current_type_environment.insert({ *decl, GuaranteeSet() });
		conditionally_raise_error<UnsupportedConstructError>(!insertion.second, "hiding variable declaration of outer scope not supported");
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

void TypeChecker::check_choice(const Choice& choice) {
	TypeEnv pre_types = this->current_type_environment;
	std::vector<TypeEnv> post_types;
	post_types.reserve(choice.branches.size());

	// compute typing of each branch individually
	for (const auto& branch : choice.branches) {
		this->current_type_environment = pre_types;
		assert(branch);
		branch->accept(*this);
		post_types.push_back(std::move(this->current_type_environment));
	}

	// merge type info: guarantees they agree on
	TypeEnv result = std::move(post_types.back());
	post_types.pop_back();
	while (!post_types.empty()) {
		result = intersection(result, post_types.back());
		post_types.pop_back();
	}

	this->current_type_environment = std::move(result);
}

void TypeChecker::check_loop(const Loop& loop) {
	TypeEnv pre_types = this->current_type_environment;
	assert(loop.body);
	loop.body->accept(*this);

	// compute the largest fixpoint of types that can go into the loop and survive; starting from the pre type environment
	while (!equals(pre_types, this->current_type_environment)) {
		pre_types = prtypes::intersection(pre_types, this->current_type_environment);
		this->current_type_environment = pre_types;
		loop.body->accept(*this);
	}
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
