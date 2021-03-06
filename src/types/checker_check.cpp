#include "types/checker.hpp"
#include "types/error.hpp"
#include "types/util.hpp"
#include "cola/util.hpp"
#include <iostream>

using namespace cola;
using namespace prtypes;

// TODO: remove if/while (for expressions containing pointers)

void debug_type_env(const TypeEnv& env, std::string note="") {
	auto print_sstate = [](const SymbolicState& symbolic_state) {
		std::cout << "{ ";
		bool first = true;
		for (const auto& state : symbolic_state.origin) {
			if (!first) std::cout << ", ";
			first = false;
			std::cout << state->name;
		}
		std::cout << " }";
	};
	std::cout << "Type Env " << note << std::endl;
	for (const auto& [decl, type] : env) {
		std::cout << "   - " << decl.get().name << ": ";
		bool first = true;
		for (const auto& state : type.states) {
			if (first) {
				first = false;
			} else {
				std::cout << "++";
			}
			print_sstate(*state);
		}
		std::cout << "   ";
		if (type.is_valid) {
			std::cout << " (valid)";
		}
		if (type.is_active) {
			std::cout << " (active)";
		}
		if (type.is_local) {
			std::cout << " (local)";
		}
		if (type.is_transient) {
			std::cout << " (transient)";
		}
		std::cout << std::endl;
	}
}


std::unique_ptr<Assert> make_assert_from_invariant(const Invariant* inv) {
	if (inv) {
		return std::make_unique<Assert>(cola::copy(*inv));
	} else {
		return std::make_unique<Assert>(std::make_unique<InvariantExpression>(std::make_unique<BooleanValue>(true)));
	}
}

void TypeChecker::check_annotated_statement(const AnnotatedStatement& stmt) {
	if (typeid(stmt) == typeid(IfThenElse)) {
		// allow IfThenElse to have annotations
		// TODO: implement a less hacky way
		return;
	}
	conditionally_raise_error<UnsupportedConstructError>(stmt.annotation != nullptr, "annotations are not supported, use 'assert' statements instead");
}


bool TypeChecker::is_pointer_valid(const VariableDeclaration& variable) {
	assert(variable.type.sort == Sort::PTR);
	assert(prtypes::has_binding(current_type_environment, variable));
	return current_type_environment.at(variable).is_valid;
}


void TypeChecker::check_skip(const Skip& /*skip*/) {
	// do nothing
}

void TypeChecker::check_malloc(const Malloc& /*malloc*/, const VariableDeclaration& ptr) {
	conditionally_raise_error<UnsupportedConstructError>(ptr.is_shared, "allocations must not target shared variables");
	assert(prtypes::has_binding(current_type_environment, ptr));
	current_type_environment.at(ptr) = type_context.local_type;
	// debug_type_env(this->current_type_environment, "post malloc");
}

void TypeChecker::check_enter(const Enter& enter, std::vector<std::reference_wrapper<const VariableDeclaration>> params) {
	// std::cout << std::endl << std::endl << ">>>>>> ENTER: "; cola::print(enter, std::cout);
	// debug_type_env(this->current_type_environment);

	// check safe
	SimulationEngine::VariableDeclarationSet invalid;
	for (const auto& variable : params) {
		if (variable.get().type.sort == Sort::PTR && !is_pointer_valid(variable)) {
			invalid.insert(variable);
		}
	}
	bool is_safe_call = type_context.observer_store.simulation.is_safe(enter, params, invalid);
	conditionally_raise_error<UnsafeCallError>(!is_safe_call, enter);

	// check retire for active
	if (&enter.decl == &type_context.observer_store.retire_function) {
		assert(params.size() == 1);
		assert(params.at(0).get().type.sort == Sort::PTR);
		assert(prtypes::has_binding(current_type_environment, params.at(0)));
		bool arg_is_valid = is_pointer_valid(params.at(0));
		bool arg_is_active = this->current_type_environment.at(params.at(0)).is_active;
		arg_is_active |= this->current_type_environment.at(params.at(0)).is_local > 0;
		conditionally_raise_error<UnsafeCallError>(!arg_is_valid, enter, "invalid argument");
		conditionally_raise_error<UnsafeCallError>(!arg_is_active, enter, "argument not active");
	}

	// update types
	this->current_type_environment = type_post(this->current_type_environment, enter);

	// std::cout << "done";
	// debug_type_env(this->current_type_environment);
	// std::cout << "<<<<<<" << std::endl << std::endl;
}

void TypeChecker::check_exit(const Exit& exit) {
	// std::cout << std::endl << std::endl << ">>>>>> EXIT: "; cola::print(exit, std::cout);
	// debug_type_env(this->current_type_environment);

	this->current_type_environment = type_post(this->current_type_environment, exit);

	// std::cout << "done";
	// debug_type_env(this->current_type_environment);
	// std::cout << "<<<<<<" << std::endl << std::endl;
}

void TypeChecker::check_return(const Return& /*retrn*/, const VariableDeclaration& var) {
	conditionally_raise_error<UnsupportedConstructError>(var.type.sort == Sort::PTR, "returning pointers is not supported");
	for (auto& [decl, type] : this->current_type_environment) {
		type = type_context.default_type; // TODO: does this work?
	}
}

void TypeChecker::check_break(const Break& /*brk*/) {
	this->break_envs.push_back(this->current_type_environment);
	
	// result is universal typeenv to avoid restricting types unnecessarily
	for (auto& [decl, type] : this->current_type_environment) {
		type = type_context.empty_type;
	}

	// debug_type_env(this->current_type_environment, "post break");
}


void TypeChecker::check_assume_nonpointer(const Assume& /*assume*/, const Expression& /*expr*/) {
	// do nothing
}

void TypeChecker::check_assume_pointer(const Assume& assume, const VariableDeclaration& lhs, BinaryExpression::Operator op, const VariableDeclaration& rhs) {
	assert(assume.expr);
	if (lhs.type.sort == Sort::PTR && op == BinaryExpression::Operator::EQ) {
		assert(prtypes::has_binding(current_type_environment, lhs));
		assert(prtypes::has_binding(current_type_environment, rhs));
		
		conditionally_raise_error<UnsafeAssumeError>(!is_pointer_valid(lhs), assume, lhs);
		conditionally_raise_error<UnsafeAssumeError>(!is_pointer_valid(rhs), assume, rhs);
		
		Type sum = prtypes::type_union(current_type_environment.at(lhs), current_type_environment.at(rhs));
		sum = prtypes::type_remove_local(sum);
		current_type_environment.at(lhs) = sum;
		current_type_environment.at(rhs) = sum;

	} else {
		// do nothing
	}
}

void TypeChecker::check_assume_pointer(const cola::Assume& assume, const cola::VariableDeclaration& lhs, cola::BinaryExpression::Operator op, const cola::Dereference& /*rhs_deref*/, const cola::VariableDeclaration& rhs_var) {
	if (lhs.type.sort == Sort::PTR && op == BinaryExpression::Operator::EQ) {
		conditionally_raise_error<UnsafeAssumeError>(!is_pointer_valid(lhs), assume, lhs);
	}

	conditionally_raise_error<UnsafeAssumeError>(!is_pointer_valid(rhs_var), assume, rhs_var);
	// rely on preprocessing to have inserted an assertion for rhs_deref result to be active and thus valid
}

void TypeChecker::check_assume_pointer(const Assume& /*assume*/, const VariableDeclaration& /*lhs*/, BinaryExpression::Operator /*op*/, const NullValue& /*rhs*/) {
	// do nothing
}

void TypeChecker::check_assume_pointer(const cola::Assume& assume, const cola::Dereference& lhs_deref, const cola::VariableDeclaration& lhs_var, cola::BinaryExpression::Operator /*op*/, const cola::Expression& /*rhs*/) {
	assert(lhs_deref.sort() != Sort::PTR);
	conditionally_raise_error<UnsafeAssumeError>(!is_pointer_valid(lhs_var), assume, lhs_var);
}

void TypeChecker::check_assert_nonpointer(const cola::Assert& /*assert*/, const cola::Expression& /*expr*/) {
	// do nothing
}

void TypeChecker::check_assert_pointer(const Assert& /*assert*/, const VariableDeclaration& lhs, BinaryExpression::Operator op, const VariableDeclaration& rhs) {
	if (op == BinaryExpression::Operator::EQ) {
		assert(prtypes::has_binding(current_type_environment, lhs));
		assert(prtypes::has_binding(current_type_environment, rhs));
		
		Type sum = prtypes::type_union(current_type_environment.at(lhs), current_type_environment.at(rhs));
		sum = prtypes::type_remove_active(sum);
		current_type_environment.at(lhs) = sum;
		current_type_environment.at(rhs) = sum;

	} else {
		raise_error<UnsupportedConstructError>("unsupported comparison operator in assertions; must be '=='");
	}
}

void TypeChecker::check_assert_pointer(const Assert& /*assert*/, const VariableDeclaration& lhs, BinaryExpression::Operator op, const NullValue& /*rhs*/) {
	if (op == BinaryExpression::Operator::EQ) {
		assert(prtypes::has_binding(current_type_environment, lhs));
		current_type_environment.at(lhs) = prtypes::type_remove_local(current_type_environment.at(lhs));

	} else {
		raise_error<UnsupportedConstructError>("unsupported comparison operator in assertions; must be '=='");
	}
}

void TypeChecker::check_assert_active(const Assert& /*assertion*/, const VariableDeclaration& ptr) {
	// std::cout << std::endl << std::endl << ">>>>>> ASSERT ACTIVE: "; cola::print(assertion, std::cout);
	// debug_type_env(this->current_type_environment);

	current_type_environment.at(ptr) = prtypes::type_add_active(current_type_environment.at(ptr));

	// std::cout << "done";
	// debug_type_env(this->current_type_environment);
	// std::cout << "<<<<<<" << std::endl << std::endl;
}

void TypeChecker::check_assert_active(const Assert& /*assert*/, const Dereference& /*deref*/, const VariableDeclaration& /*ptr*/) {
	// do nothing
}


void TypeChecker::check_assign_pointer(const Assignment& /*node*/, const VariableDeclaration& lhs, const VariableDeclaration& rhs) {
//	std::cout << "ASSIGN ptr = ptr: "; cola::print(node, std::cout);
//	debug_type_env(this->current_type_environment);

	assert(prtypes::has_binding(current_type_environment, lhs));
	assert(prtypes::has_binding(current_type_environment, rhs));

	Type result = prtypes::type_remove_local(current_type_environment.at(rhs));

	current_type_environment.at(lhs) = result;
	current_type_environment.at(rhs) = result;
	
//	std::cout << "done ASSIGN";
//	debug_type_env(this->current_type_environment);
}

void TypeChecker::check_assign_pointer(const Assignment& /*node*/, const VariableDeclaration& lhs, const NullValue& /*rhs*/) {
	assert(prtypes::has_binding(current_type_environment, lhs));
	current_type_environment.at(lhs) = type_context.default_type;
}

void TypeChecker::check_assign_pointer(const Assignment& assignment, const Dereference& lhs_deref, const VariableDeclaration& lhs_var, const VariableDeclaration& rhs) {
	assert(prtypes::has_binding(current_type_environment, lhs_var));
	assert(prtypes::has_binding(current_type_environment, rhs));

	conditionally_raise_error<UnsafeDereferenceError>(!is_pointer_valid(lhs_var), assignment, lhs_deref, lhs_var);
	current_type_environment.at(rhs) = prtypes::type_remove_local(current_type_environment.at(rhs));
}

void TypeChecker::check_assign_pointer(const Assignment& /*node*/, const Dereference& /*lhs_deref*/, const VariableDeclaration& /*lhs_var*/, const NullValue& /*rhs*/) {
	// do nothing
}

void TypeChecker::check_assign_pointer(const Assignment& assignment, const VariableDeclaration& lhs, const Dereference& rhs_deref, const VariableDeclaration& rhs_var) {
	assert(prtypes::has_binding(current_type_environment, lhs));
	assert(prtypes::has_binding(current_type_environment, rhs_var));

	conditionally_raise_error<UnsafeDereferenceError>(!is_pointer_valid(rhs_var), assignment, rhs_deref, rhs_var);
	current_type_environment.at(lhs) = type_context.default_type;
}

void TypeChecker::check_assign_nonpointer(const Assignment& /*node*/, const Expression& /*lhs*/, const Expression& /*rhs*/) {
	// do nothing
}

void TypeChecker::check_assign_nonpointer(const Assignment& assignment, const Dereference& lhs_deref, const VariableDeclaration& lhs_var, const VariableDeclaration& /*rhs*/) {
	assert(prtypes::has_binding(current_type_environment, lhs_var));
	conditionally_raise_error<UnsafeDereferenceError>(!is_pointer_valid(lhs_var), assignment, lhs_deref, lhs_var);
}

void TypeChecker::check_assign_nonpointer(const Assignment& assignment, const VariableDeclaration& /*lhs*/, const Dereference& rhs_deref, const VariableDeclaration& rhs_var) {
//	std::cout << "DEREF data = sel: "; cola::print(assignment, std::cout);
//	debug_type_env(this->current_type_environment);
	assert(prtypes::has_binding(current_type_environment, rhs_var));
	conditionally_raise_error<UnsafeDereferenceError>(!is_pointer_valid(rhs_var), assignment, rhs_deref, rhs_var);
}


void TypeChecker::check_angel_choose(bool active) {
	conditionally_raise_error<TypeCheckError>(!!current_angel, "Only one angel allocation per function execution supported (don't put it into loops).");
	current_angel = std::make_unique<VariableDeclaration>("§A§", type_context.observer_store.retire_function.args.at(0)->type, false);
	assert(!prtypes::has_binding(current_type_environment, *current_angel));
	Type type = active ? type_context.active_type : type_context.default_type;
	current_type_environment.insert({ *current_angel, type });

	// debug_type_env(current_type_environment, "@angle(choose) post");
}

void TypeChecker::check_angel_active() {
	conditionally_raise_error<TypeCheckError>(!current_angel, "Angel used but not allocated.");
	assert(prtypes::has_binding(current_type_environment, *current_angel));

	// debug_type_env(current_type_environment, "@angle(active) pre");

	Type type = current_type_environment.at(*current_angel);
	type = prtypes::type_add_active(type);
	current_type_environment.at(*current_angel) = type;

	// debug_type_env(current_type_environment, "@angle(active) post");
}

void TypeChecker::check_angel_contains(const VariableDeclaration& ptr) {
	conditionally_raise_error<TypeCheckError>(!current_angel, "Angel used but not allocated.");
	assert(prtypes::has_binding(current_type_environment, *current_angel));
	assert(prtypes::has_binding(current_type_environment, ptr));

	// debug_type_env(current_type_environment, "@angle(contains(" + ptr.name + ")) pre");

	Type sum = prtypes::type_union(current_type_environment.at(*current_angel), current_type_environment.at(ptr));
	current_type_environment.at(ptr) = sum;

	// debug_type_env(current_type_environment, "@angle(contains(" + ptr.name + ")) post");
}


void TypeChecker::check_scope(const Scope& scope) {
	// populate current_type_environment with default type for declared pointer variables
	for (const auto& decl : scope.variables) {
		if (decl->type.sort == Sort::PTR) {
			auto insertion = current_type_environment.insert({ *decl, type_context.default_type });
			conditionally_raise_error<UnsupportedConstructError>(!insertion.second, "hiding variable declaration of outer scope not supported");
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

void TypeChecker::check_sequence(const Sequence& sequence) {
	assert(sequence.first);
	assert(sequence.second);
	sequence.first->accept(*this);
	sequence.second->accept(*this);
	// widening is applied directly at the commands
}

void TypeChecker::check_atomic_begin() {
	// do nothing
}

void TypeChecker::check_atomic_end() {
	// handle transient types on local pointers, reset shared pointers to default type
	for (auto& [decl, type] : current_type_environment) {
		if (decl.get().is_shared) {
			type = type_context.default_type;
		} else if (type.is_transient) {
			type = prtypes::type_closure(type);
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
		result = prtypes::type_intersection(result, post_types.back());
		post_types.pop_back();
	}

	this->current_type_environment = std::move(result);
}

void TypeChecker::check_ite(const IfThenElse& ite) {
	// on-the-fly:
	// handle '@invariant(<inv>) if (<cond>) { <if> } else { <else> }'
	// as     'choose { atomic { assert(<inv>); assume(<cond>); }; <if> }{ atomic { assert(<inv>); assume(!<cond>); }; <else> }'
	assert(ite.expr);
	auto prolog_positive = std::make_unique<Atomic>(std::make_unique<Scope>(std::make_unique<Sequence>(make_assert_from_invariant(ite.annotation.get()), std::make_unique<Assume>(cola::copy(*ite.expr)))));
	auto prolog_negative = std::make_unique<Atomic>(std::make_unique<Scope>(std::make_unique<Sequence>(make_assert_from_invariant(ite.annotation.get()), std::make_unique<Assume>(cola::negate(*ite.expr)))));
	TypeEnv pre_types = this->current_type_environment;

	// compute typing of true branch
	try {
		prolog_positive->accept(*this);
	} catch (UnsafeAssumeError err) {
		std::cout << "ITE FAILED: " << err.what() << std::endl;
		throw std::logic_error("not yet implemented: TypeChecker::check_ite(const IfThenElse&), translation from UnsafeAssumeError to UnsafeIteConditionError");
	}
	assert(ite.ifBranch);
	ite.ifBranch->accept(*this);
	TypeEnv post_true = std::move(this->current_type_environment);

	// compute typing of false branch
	this->current_type_environment = pre_types;
	try {
		prolog_negative->accept(*this);
	} catch (UnsafeAssumeError err) {
		std::cout << "ITE FAILED: " << err.what() << std::endl;
		throw std::logic_error("not yet implemented: TypeChecker::check_ite(const IfThenElse&), translation from UnsafeAssumeError to UnsafeIteConditionError");
	}
	assert(ite.elseBranch);
	ite.elseBranch->accept(*this);
	TypeEnv post_false = std::move(this->current_type_environment);

	this->current_type_environment = prtypes::type_intersection(post_true, post_false);
}

void TypeChecker::check_loop(const Loop& loop) {
	// std::cout << "ENTERING WHILE" << std::endl;
//	debug_type_env(this->current_type_environment);
	assert(loop.body);
	TypeEnv pre_types;

	conditionally_raise_error<TypeCheckError>(!this->break_envs.empty(), "'break' must not jump over loops");
	do {
//		std::cout << "========DOING WHILE" << std::endl;
//		debug_type_env(this->current_type_environment);
		assert(this->break_envs.empty());
		pre_types = this->current_type_environment;
		loop.body->accept(*this);
		this->current_type_environment = prtypes::type_intersection(pre_types, this->current_type_environment);
		
		// conditionally_raise_error<TypeCheckError>(!this->break_envs.empty(), "'break' must not appear in (conditional) loops");
		while (!this->break_envs.empty()) {
			this->current_type_environment = prtypes::type_intersection(this->current_type_environment, std::move(this->break_envs.back()));
			this->break_envs.pop_back();
		}

	} while (!prtypes::equals(pre_types, this->current_type_environment));
//	std::cout << "EXITING WHILE" << std::endl;
//	debug_type_env(this->current_type_environment);
}

void TypeChecker::check_while(const While& whl) {
	// std::cout << "ENTERING WHILE " << whl.id << std::endl;
	// debug_type_env(this->current_type_environment);
	assert(whl.expr);
	assert(whl.body);

	const Expression& expr = *whl.expr;
	conditionally_raise_error<UnsupportedConstructError>(typeid(expr) != typeid(BooleanValue), "unsupported 'while' condition; expected a boolean value");
	conditionally_raise_error<UnsupportedConstructError>(!static_cast<const BooleanValue&>(expr).value, "unsupported 'while' condition; expected value 'true'");

	std::unique_ptr<TypeEnv> result;

	// apply loop rule, but peel breaking iterations
	TypeEnv pre_types;
	do {
		// std::cout << "========DOING WHILE " << whl.id << std::endl;
		// debug_type_env(this->current_type_environment);
		pre_types = this->current_type_environment;
		whl.body->accept(*this);
		this->current_type_environment = prtypes::type_intersection(pre_types, this->current_type_environment);

		conditionally_raise_error<TypeCheckError>(this->break_envs.size() == 0, "'while (true)' does not 'break'");
		if (!result) {
			result = std::make_unique<TypeEnv>();
			*result = std::move(this->break_envs.back());
			this->break_envs.pop_back();
		}
		while (!this->break_envs.empty()) {
			*result = prtypes::type_intersection(*result, std::move(this->break_envs.back()));
			this->break_envs.pop_back();
		}

	} while (!prtypes::equals(pre_types, this->current_type_environment));

	assert(this->break_envs.empty());
	this->current_type_environment = std::move(*result);
	// std::cout << "EXITING WHILE" << std::endl;
	// debug_type_env(this->current_type_environment);

	// // on-the-fly handle 'while (<cond>) { <body> }' as 'loop { assume(<cond>); <body> }; assume(!<cond>);'
	// auto assume_positive = std::make_unique<Assume>(cola::copy(*whl.expr));
	// auto assume_negative = std::make_unique<Assume>(cola::negate(*whl.expr));

	// // apply loop rule
	// assert(whl.body);
	// TypeEnv pre_types;
	// std::vector<TypeEnv> breaks = std::move(this->break_envs);

	// do {
	// 	pre_types = this->current_type_environment;
	// 	try {
	// 		assume_positive->accept(*this);
	// 	} catch (UnsafeAssumeError err) {
	// 		std::cout << "WHILE FAILED: " << err.what() << std::endl;
	// 		throw std::logic_error("not yet implemented: TypeChecker::check_while(const While&), translation from UnsafeAssumeError to UnsafeWhileConditionError");
	// 	}
	// 	whl.body->accept(*this);
	// 	this->current_type_environment = prtypes::intersection(pre_types, this->current_type_environment);

	// } while (!prtypes::equals(pre_types, this->current_type_environment));

	// try {
	// 	assume_negative->accept(*this);
	// } catch (UnsafeAssumeError err) {
	// 	std::cout << "WHILE FAILED: " << err.what() << std::endl;
	// 	throw std::logic_error("not yet implemented: TypeChecker::check_while(const While&), translation from UnsafeAssumeError to UnsafeWhileConditionError");
	// }
}

void TypeChecker::check_interface_function(const Function& function) {
	std::cout << "[" << function.name << "]" << std::endl;
	function.body->accept(*this);

	// clean up
	if (current_angel) {
		assert(prtypes::has_binding(current_type_environment, *current_angel));
		current_type_environment.erase(*current_angel);
	}
	current_angel.release();
}

void TypeChecker::check_program(const Program& program) {
	// TODO: what about program.initializer?
	
	// populate current_type_environment with empty guarantees for shared pointer variables
	for (const auto& decl : program.variables) {
		auto insertion = current_type_environment.insert({ *decl, type_context.default_type });
		conditionally_raise_error<UnsupportedConstructError>(!insertion.second, "multiple occurence of the same shared variable declaration");
	}

	// type check functions
	for (const auto& function : program.functions) {
		function->accept(*this);
	}
}
