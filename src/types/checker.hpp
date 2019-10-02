#pragma once
#ifndef PRTYPES_CHECKER
#define PRTYPES_CHECKER

#include <optional>
#include "cola/ast.hpp"
#include "types/types.hpp"
#include "types/simulation.hpp"


namespace prtypes {

	class TypeChecker final : public cola::Visitor {
		private:
			const cola::Program& program;
			const TypeContext& type_context;

		public:
			TypeChecker(const cola::Program& prog, const TypeContext& context) : program(prog), type_context(context) {}

			bool is_well_typed(const cola::AstNode& node) {
				// TODO: proper exception handling
				node.accept(*this);
				return true;
			}

			bool is_well_typed() {
				return is_well_typed(program);
			}

		public: // cola::Visitor interface
			void visit(const cola::VariableDeclaration& node) override;
			void visit(const cola::Expression& node) override;
			void visit(const cola::BooleanValue& node) override;
			void visit(const cola::NullValue& node) override;
			void visit(const cola::EmptyValue& node) override;
			void visit(const cola::MaxValue& node) override;
			void visit(const cola::MinValue& node) override;
			void visit(const cola::NDetValue& node) override;
			void visit(const cola::VariableExpression& node) override;
			void visit(const cola::NegatedExpression& node) override;
			void visit(const cola::BinaryExpression& node) override;
			void visit(const cola::Dereference& node) override;
			void visit(const cola::InvariantExpression& node) override;
			void visit(const cola::InvariantActive& node) override;
			void visit(const cola::Sequence& node) override;
			void visit(const cola::Scope& node) override;
			void visit(const cola::Atomic& node) override;
			void visit(const cola::Choice& node) override;
			void visit(const cola::IfThenElse& node) override;
			void visit(const cola::Loop& node) override;
			void visit(const cola::While& node) override;
			void visit(const cola::Skip& node) override;
			void visit(const cola::Break& node) override;
			void visit(const cola::Continue& node) override;
			void visit(const cola::Assume& node) override;
			void visit(const cola::Assert& node) override;
			void visit(const cola::AngelChoose& node) override;
			void visit(const cola::AngelActive& node) override;
			void visit(const cola::AngelContains& node) override;
			void visit(const cola::Return& node) override;
			void visit(const cola::Malloc& node) override;
			void visit(const cola::Assignment& node) override;
			void visit(const cola::Enter& node) override;
			void visit(const cola::Exit& node) override;
			void visit(const cola::Macro& node) override;
			void visit(const cola::CompareAndSwap& node) override;
			void visit(const cola::Function& node) override;
			void visit(const cola::Program& node) override;
			void visit_command_begin();
			void visit_command_end();

		private: // type check interface (called from visitor interface)
			void check_annotated_statement(const cola::AnnotatedStatement& stmt);
			void check_skip(const cola::Skip& skip);
			void check_malloc(const cola::Malloc& malloc, const cola::VariableDeclaration& ptr);
			void check_enter(const cola::Enter& enter, std::vector<std::reference_wrapper<const cola::VariableDeclaration>> params);
			void check_exit(const cola::Exit& exit);
			void check_break(const cola::Break& brk);
			void check_return(const cola::Return& retrn, const cola::VariableDeclaration& var);
			void check_assume_nonpointer(const cola::Assume& assume, const cola::Expression& expr);
			void check_assume_pointer(const cola::Assume& assume, const cola::VariableDeclaration& lhs, cola::BinaryExpression::Operator op, const cola::VariableDeclaration& rhs);
			void check_assume_pointer(const cola::Assume& assume, const cola::VariableDeclaration& lhs, cola::BinaryExpression::Operator op, const cola::Dereference& rhs_deref, const cola::VariableDeclaration& rhs_var);
			void check_assume_pointer(const cola::Assume& assume, const cola::VariableDeclaration& lhs, cola::BinaryExpression::Operator op, const cola::NullValue& rhs);
			void check_assume_pointer(const cola::Assume& assume, const cola::Dereference& lhs_deref, const cola::VariableDeclaration& lhs_var, cola::BinaryExpression::Operator op, const cola::Expression& rhs);
			void check_assert_nonpointer(const cola::Assert& assert, const cola::Expression& expr);
			void check_assert_pointer(const cola::Assert& assert, const cola::VariableDeclaration& lhs, cola::BinaryExpression::Operator op, const cola::VariableDeclaration& rhs);
			void check_assert_pointer(const cola::Assert& assert, const cola::VariableDeclaration& lhs, cola::BinaryExpression::Operator op, const cola::NullValue& rhs);
			void check_assert_active(const cola::Assert& assert, const cola::VariableDeclaration& ptr);
			void check_assert_active(const cola::Assert& assert, const cola::Dereference& deref, const cola::VariableDeclaration& ptr);
			void check_assign_pointer(const cola::Assignment& node, const cola::VariableDeclaration& lhs, const cola::VariableDeclaration& rhs);
			void check_assign_pointer(const cola::Assignment& node, const cola::VariableDeclaration& lhs, const cola::NullValue& rhs);
			void check_assign_pointer(const cola::Assignment& node, const cola::Dereference& lhs_deref, const cola::VariableDeclaration& lhs_var, const cola::VariableDeclaration& rhs);
			void check_assign_pointer(const cola::Assignment& node, const cola::Dereference& lhs_deref, const cola::VariableDeclaration& lhs_var, const cola::NullValue& rhs);
			void check_assign_pointer(const cola::Assignment& node, const cola::VariableDeclaration& lhs, const cola::Dereference& rhs_deref, const cola::VariableDeclaration& rhs_var);
			void check_assign_nonpointer(const cola::Assignment& node, const cola::Expression& lhs, const cola::Expression& rhs);
			void check_assign_nonpointer(const cola::Assignment& node, const cola::Dereference& lhs_deref, const cola::VariableDeclaration& lhs_var, const cola::VariableDeclaration& rhs);
			void check_assign_nonpointer(const cola::Assignment& node, const cola::VariableDeclaration& lhs, const cola::Dereference& rhs_deref, const cola::VariableDeclaration& rhs_var);
			void check_angel_choose(bool active);
			void check_angel_active();
			void check_angel_contains(const cola::VariableDeclaration& ptr);
			void check_atomic_begin();
			void check_atomic_end();
			void check_scope(const cola::Scope& node);
			void check_sequence(const cola::Sequence& node);
			void check_choice(const cola::Choice& choice);
			void check_ite(const cola::IfThenElse& ite);
			void check_loop(const cola::Loop& loop);
			void check_while(const cola::While& whl);
			void check_interface_function(const cola::Function& function);
			void check_program(const cola::Program& program);

		private: // helpers
			TypeEnv current_type_environment;
			bool inside_atomic = false;
			const cola::Assert* current_assert;
			std::vector<TypeEnv> break_envs;
			std::unique_ptr<cola::VariableDeclaration> current_angel;

			struct VariableOrDereferenceOrNull {
				std::optional<const cola::VariableDeclaration*> var;
				std::optional<const cola::Dereference*> deref;
				std::optional<const cola::NullValue*> null;
				std::optional<const cola::Expression*> value;
			};
			struct FlatBinaryExpression {
				// const cola::VariableDeclaration* lhs;
				VariableOrDereferenceOrNull lhs;
				cola::BinaryExpression::Operator op;
				VariableOrDereferenceOrNull rhs;
			};

			bool is_pointer_valid(const cola::VariableDeclaration& variable);
			const cola::VariableDeclaration& expression_to_variable(const cola::Expression& expression);
			VariableOrDereferenceOrNull expression_to_variable_or_dereference_or_null_or_value(const cola::Expression& expression);
			FlatBinaryExpression expression_to_flat_binary_expression(const cola::Expression& expression);
	};

} // namespace prtypes

#endif