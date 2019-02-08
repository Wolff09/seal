#pragma once
#ifndef PRTYPES_CHECK_IMPL
#define PRTYPES_CHECK_IMPL

#include <memory>
#include <string>
#include "types/check.hpp"
#include "cola/ast.hpp"


namespace prtypes {

inline void raise_type_error(std::string cause) {
	throw std::logic_error("Type check error: " + cause + ".");
}

inline bool entails_valid(const GuaranteeSet& guarantees) {
	for (const auto& guarantee : guarantees) {
		if (guarantee.get().entails_validity) {
			return true;
		}
	}
	return false;
}

struct TypeCheckStatementVisitor final : public cola::Visitor {
	const GuaranteeTable& guarantee_table;
	const cola::Program* current_program;
	const cola::Function* current_function;
	TypeEnv current_type_environment;
	bool in_atomic = false; // handle commands occuring outside an atomic block as if they were wrapped in one

	TypeCheckStatementVisitor(const GuaranteeTable& table) : guarantee_table(table) {}
	bool is_well_typed(const cola::Program& program);
	void visit(const cola::VariableDeclaration& node) override;
	void visit(const cola::Expression& node) override;
	void visit(const cola::BooleanValue& node) override;
	void visit(const cola::NullValue& node) override;
	void visit(const cola::EmptyValue& node) override;
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
	void visit(const cola::Return& node) override;
	void visit(const cola::Malloc& node) override;
	void visit(const cola::Assignment& node) override;
	void visit(const cola::Enter& node) override;
	void visit(const cola::Exit& node) override;
	void visit(const cola::Macro& node) override;
	void visit(const cola::CompareAndSwap& node) override;
	void visit(const cola::Function& node) override;
	void visit(const cola::Program& node) override;
};

} // namespace prtypes

#endif
