#include "types/rmraces.hpp"
#include "types/cave.hpp"
#include "cola/util.hpp"
#include "types/inference.hpp"
#include <iostream>

using namespace cola;
using namespace prtypes;

const bool ASSUME_SHARED_ACTIVE = true;
const bool CHECK_SINGLETON_PATH = false;


std::unique_ptr<BinaryExpression> make_negated_expression(std::unique_ptr<Expression> expr) {
	// TODO: properly add this functionality to cola/util.hpp
	Expression* expr_ptr = expr.release();
	std::unique_ptr<BinaryExpression> bineary_expression;
	bineary_expression.reset(static_cast<BinaryExpression*>(expr_ptr));
	switch (bineary_expression->op) {
		case BinaryExpression::Operator::EQ: bineary_expression->op = BinaryExpression::Operator::NEQ; break;
		case BinaryExpression::Operator::NEQ: bineary_expression->op = BinaryExpression::Operator::EQ; break;
		default: raise_error<UnsupportedConstructError>("unsupported binary operator");
	}
	return bineary_expression;
}

struct NameCollectorVisitor final : public Visitor {
	std::set<std::string> names;
	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const EmptyValue&)"); }
	void visit(const MaxValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const MaxValue&)"); }
	void visit(const MinValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const MinValue&)"); }
	void visit(const NDetValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const NDetValue&)"); }
	void visit(const VariableExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const VariableExpression&)"); }
	void visit(const NegatedExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const BinaryExpression&)"); }
	void visit(const Dereference& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const Dereference&)"); }
	void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const InvariantActive&)"); }
	void visit(const Program& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const Program&)"); }

	void visit(const Skip& /*node*/) override { /* do nothing */ }
	void visit(const Break& /*node*/) override { /* do nothing */ }
	void visit(const Continue& /*node*/) override { /* do nothing */ }
	void visit(const Assume& /*node*/) override { /* do nothing */ }
	void visit(const Assert& /*node*/) override { /* do nothing */ }
	void visit(const Return& /*node*/) override { /* do nothing */ }
	void visit(const Malloc& /*node*/) override { /* do nothing */ }
	void visit(const Assignment& /*node*/) override { /* do nothing */ }
	void visit(const Enter& /*node*/) override { /* do nothing */ }
	void visit(const Exit& /*node*/) override { /* do nothing */ }
	void visit(const Macro& /*node*/) override { /* do nothing */ }
	void visit(const CompareAndSwap& /*node*/) override { /* do nothing */ }

	void visit(const VariableDeclaration& decl) override {
		this->names.insert(decl.name);
	}
	void visit(const Sequence& node) override { node.first->accept(*this); node.second->accept(*this); }
	void visit(const Atomic& node) override { node.body->accept(*this); }
	void visit(const Choice& node) override { for (const auto& branch : node.branches) { branch->accept(*this); } }
	void visit(const IfThenElse& node) override { node.ifBranch->accept(*this); node.elseBranch->accept(*this); }
	void visit(const Loop& node) override { node.body->accept(*this); }
	void visit(const While& node) override { node.body->accept(*this); }
	void visit(const Scope& node) override {
		for (const auto& var : node.variables) {
			var->accept(*this);
		}
		node.body->accept(*this);
	}
	void visit(const Function& function) override {
		for (const auto& arg : function.args) {
			arg->accept(*this);
		}
		function.body->accept(*this);
	}
};

struct LocalExpressionVisitor final : public Visitor {
	bool result = true;
	std::set<const VariableDeclaration*> decls;
	void visit(const VariableDeclaration& decl) {
		if (decl.is_shared) {
			result = false;
		}
		decls.insert(&decl);
	}
	void visit(const Dereference& deref) {
		// memory access may not be local
		assert(deref.expr);
		deref.expr->accept(*this);
		result = false;
	}
	void visit(const BooleanValue& /*node*/) { /* do nothing */ }
	void visit(const NullValue& /*node*/) { /* do nothing */ }
	void visit(const EmptyValue& /*node*/) { /* do nothing */ }
	void visit(const MaxValue& /*node*/) { /* do nothing */ }
	void visit(const MinValue& /*node*/) { /* do nothing */ }
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

std::set<const VariableDeclaration*> collect_variables(const Expression& expr) {
	LocalExpressionVisitor visitor;
	expr.accept(visitor);
	return std::move(visitor.decls);
}

struct AssertionInsertionVisitor : public NonConstVisitor {
	const Command& to_find;
	std::unique_ptr<Statement> to_insert;
	const bool insert_after;
	Statement* insertion_parent = nullptr; // may come in handy to undo insertion
	Function* current_function;
	Function* found_function;
	Statement* inserted;
	Statement* found;
	std::unique_ptr<Statement>* owner_found;
	std::unique_ptr<Statement>* owner_inserted;
	AssertionInsertionVisitor(const Command& to_find, std::unique_ptr<Statement> to_insert, bool insert_after) : to_find(to_find), to_insert(std::move(to_insert)), insert_after(insert_after) {}

	void visit(VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(VariableDeclaration&)"); }
	void visit(Expression& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(Expression&)"); }
	void visit(BooleanValue& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(BooleanValue&)"); }
	void visit(NullValue& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(NullValue&)"); }
	void visit(EmptyValue& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(EmptyValue&)"); }
	void visit(MaxValue& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(MaxValue&)"); }
	void visit(MinValue& /*node*/) { throw std::logic_error("Unexpected invocation: AssertionInsertionVisitor::visit(MinValue&)"); }
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
		this->inserted = this->to_insert.get();
		this->found = found.get();
		assert(this->found);
		std::unique_ptr<Sequence> result;
		if (this->insert_after) {
			// insert: to_find - to_insert
			result = std::make_unique<Sequence>(std::move(found), std::move(this->to_insert));
			this->owner_found = &result->first;
			this->owner_inserted = &result->second;
		} else {
			// insert: to_insert - to_find
			result = std::make_unique<Sequence>(std::move(this->to_insert), std::move(found));
			this->owner_found = &result->second;
			this->owner_inserted = &result->first;
		}
		assert(result);
		this->insertion_parent = result.get();
		this->found_function = this->current_function;
		assert(!this->to_insert);
		assert(this->insertion_parent);
		assert(this->found_function);
		assert(this->owner_found);
		assert(this->owner_inserted);
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
		current_function = &function;
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

AssertionInsertionVisitor insert_active_assertion(Program& program, const GuaranteeTable& guarantee_table, const Command& cmd, const VariableDeclaration& var, bool insert_after_cmd=false, bool no_check=false) {
	// make insertion to insert
	auto assertion = std::make_unique<Assert>(std::make_unique<InvariantActive>(std::make_unique<VariableExpression>(var)));
	Assert& inserted_assertion = *assertion.get();

	// patch program
	AssertionInsertionVisitor visitor(cmd, std::move(assertion), insert_after_cmd);
	program.accept(visitor);
	assert(!visitor.to_insert);

	if (no_check || (ASSUME_SHARED_ACTIVE && var.is_shared)) {
		return visitor;
	}

	// check added assertion for validity
	bool is_inserted_assertion_valid = prtypes::discharge_assertions(program, guarantee_table, { inserted_assertion });
	if (!is_inserted_assertion_valid) {
		// roll back insertion, then fail
		*visitor.owner_inserted = std::make_unique<Skip>();
		raise_error<RefinementError>("could not infer valid assertion to fix pointer race");
	}
	return visitor;
}

struct AssignmentExpressionVisitor final : public Visitor {
	std::set<const VariableDeclaration*> search;
	bool result = false;
	AssignmentExpressionVisitor(std::set<const VariableDeclaration*> search_) : search(std::move(search_)) {}

	void visit(const VariableDeclaration& decl) override {
		if (search.count(&decl) > 0) {
			result = true;
		}
	}
	void visit(const VariableExpression& expr) override {
		expr.decl.accept(*this);
	}
	void visit(const Dereference& /*node*/) override { /* do nothing */ }

	void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const EmptyValue&)"); }
	void visit(const MaxValue& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const MaxValue&)"); }
	void visit(const MinValue& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const MinValue&)"); }
	void visit(const NDetValue& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const NDetValue&)"); }
	void visit(const NegatedExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const BinaryExpression&)"); }
	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Expression&)"); }
	void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const InvariantActive&)"); }
	void visit(const Sequence& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Sequence&)"); }
	void visit(const Scope& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Scope&)"); }
	void visit(const Atomic& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Atomic&)"); }
	void visit(const Choice& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Choice&)"); }
	void visit(const IfThenElse& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const IfThenElse&)"); }
	void visit(const Loop& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Loop&)"); }
	void visit(const While& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const While&)"); }
	void visit(const Skip& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Skip&)"); }
	void visit(const Break& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Break&)"); }
	void visit(const Continue& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Continue&)"); }
	void visit(const Assume& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Assume&)"); }
	void visit(const Assert& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Assert&)"); }
	void visit(const Return& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Return&)"); }
	void visit(const Malloc& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Malloc&)"); }
	void visit(const Assignment& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Assignment&)"); }
	void visit(const Enter& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Enter&)"); }
	void visit(const Exit& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Exit&)"); }
	void visit(const Macro& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Macro&)"); }
	void visit(const CompareAndSwap& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const CompareAndSwap&)"); }
	void visit(const Function& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) override { throw std::logic_error("Unexpected invocation: AssignmentExpressionVisitor::visit(const Program&)"); }
};

struct InsertionLocationFinderVisitor final : public Visitor {
	std::set<const VariableDeclaration*> variables;
	const Command& end;
	bool on_path = false;
	bool path_reset = false;
	std::vector<const Command*> path;
	std::vector<const Statement*> full_path;
	InsertionLocationFinderVisitor(std::set<const VariableDeclaration*> vars, const Command& end) : variables(std::move(vars)), end(end) {}

	struct Heureka : public std::exception {
		virtual const char* what() const noexcept { return "HEUREKA"; }
	};

	std::optional<std::vector<const Command*>> get_path(const Program& program) {
		try {
			program.accept(*this);
		} catch (Heureka) {
			return this->path;
		}
		return std::nullopt;
	}

	std::vector<const Command*> get_path_or_raise(const Program& program) {
		auto path_opt = get_path(program);
		if (path_opt.has_value()) {
			return *path_opt;
		} else {
			throw RefinementError("could not find an insertion path");
		}
	}

	std::vector<const Statement*> get_full_path_or_raise(const Program& program) {
		get_path_or_raise(program);
		return full_path;
	}

	void abort() {
		throw Heureka();
	}

	void visit(const VariableDeclaration& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const VariableDeclaration&)"); }
	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const EmptyValue&)"); }
	void visit(const MaxValue& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const MaxValue&)"); }
	void visit(const MinValue& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const MinValue&)"); }
	void visit(const NDetValue& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const NDetValue&)"); }
	void visit(const VariableExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const VariableExpression&)"); }
	void visit(const NegatedExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const BinaryExpression&)"); }
	void visit(const Dereference& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const Dereference&)"); }
	void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const InvariantActive&)"); }

	// assignment to variable => on_path = true, path.clear()
	void visit(const Skip& /*node*/) override { /* do nothing */ }
	void visit(const Break& node) override { if (on_path) { full_path.push_back(&node); } }
	void visit(const Continue& node) override { if (on_path) { full_path.push_back(&node); } }
	void visit(const Return& node) override { if (on_path) { full_path.push_back(&node); } }
	void visit(const Enter& node) override { if (on_path) { full_path.push_back(&node); } }
	void visit(const Exit& node) override { if (on_path) { full_path.push_back(&node); } }
	void visit(const Macro& node) override { if (on_path) { full_path.push_back(&node); } }
	void visit(const CompareAndSwap& node) override { if (on_path) { full_path.push_back(&node); } }
	void visit(const Assert& node) override { if (on_path) { full_path.push_back(&node); } }
	void visit(const Malloc& malloc) override {
		if (variables.count(&malloc.lhs) > 0) {
			on_path = true;
			path.clear();
			path_reset = true;
			path.push_back(&malloc);
		}
		if (on_path) {
			full_path.push_back(&malloc);
		}
	}
	void visit(const Assignment& assign) override {
		AssignmentExpressionVisitor visitor(variables);
		assert(assign.lhs);
		assign.lhs->accept(visitor);

		if (visitor.result) {
			// assignment to this->variable
			on_path = true;
			path.clear();
			full_path.clear();
			path_reset = true;
			path.push_back(&assign);
		}
		if (on_path) {
			full_path.push_back(&assign);
		}
	}
	void visit(const Assume& assume) override {
		assert(assume.expr);
		if (on_path && !is_expression_local(*assume.expr)) {
			path.push_back(&assume);
		}
		if (on_path) {
			full_path.push_back(&assume);
		}
	}

	void visit(const Sequence& seq) override {
		// handle first
		assert(seq.first);
		if (seq.first.get() == &this->end) {
			abort();
		} else {
			seq.first->accept(*this);
		}
		// handle second
		assert(seq.second);
		if (seq.second.get() == &this->end) {
			abort();
		} else {
			seq.second->accept(*this);
		}
	}
	void visit(const Scope& scope) override {
		assert(scope.body);
		if (scope.body.get() == &this->end) {
			abort();
		} else {
			scope.body->accept(*this);
		}
	}
	void visit(const Atomic& node) override {
		assert(node.body);
		node.body->accept(*this);
	}
	void visit(const Choice& node) override {
		std::vector<const Command*> copy_path = path;
		std::vector<const Statement*> copy_full_path = full_path;
		bool copy_on = on_path;
		path_reset = false;
		for (const auto& branch : node.branches) {
			assert(branch);
			branch->accept(*this);

			path = copy_path;
			full_path = copy_full_path;
			on_path = copy_on;
		}
		if (path_reset) {
			on_path = false;
			path.clear();
			full_path.clear();
		} else {
			full_path.push_back(&node);
		}
	}
	void visit(const Loop& node) override {
		std::vector<const Command*> copy_path = path;
		std::vector<const Statement*> copy_full_path = full_path;
		bool copy_on = on_path;
		
		assert(node.body);
		node.body->accept(*this);

		path = std::move(copy_path);
		on_path = copy_on;

		full_path = std::move(copy_full_path);
		full_path.push_back(&node);
	}
	void visit(const IfThenElse& /*node*/) override {
		raise_error<UnsupportedConstructError>("'if' not supported");
	}
	void visit(const While& node) override {
		auto& expr = *node.expr;
		assert(typeid(expr) == typeid(BooleanValue));

		std::vector<const Command*> copy_path = path;
		std::vector<const Statement*> copy_full_path = full_path;
		bool copy_on = on_path;
		
		assert(node.body);
		node.body->accept(*this);

		path = std::move(copy_path);
		on_path = copy_on;

		full_path = std::move(copy_full_path);
		full_path.push_back(&node);
	}
	void visit(const Function& function) override {
		on_path = false;
		if (function.kind == Function::SMR) {
			return;
		}
		assert(function.body);
		function.body->accept(*this);
	}
	void visit(const Program& program) override {
		assert(program.initializer);
		program.initializer->accept(*this);
		for (const auto& function : program.functions) {
			assert(function);
			function->accept(*this);
		}
	}
};

struct RightMovernessVisitor final : public Visitor {
	private:
	bool moves = true;
	const Function& retire_function;
	std::vector<std::reference_wrapper<const Command>> events;

	public:
	RightMovernessVisitor(const Function& retire) : retire_function(retire) {}

	bool right_moves(const SimulationEngine& engine) {
		if (!engine.is_repeated_execution_simulating(this->events)) {
			this->moves = false;
		}
		return this->moves;
	}

	void visit(const VariableDeclaration& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const VariableDeclaration&)"); }
	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const EmptyValue&)"); }
	void visit(const MaxValue& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const MaxValue&)"); }
	void visit(const MinValue& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const MinValue&)"); }
	void visit(const NDetValue& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const NDetValue&)"); }
	void visit(const VariableExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const VariableExpression&)"); }
	void visit(const NegatedExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const BinaryExpression&)"); }
	void visit(const Dereference& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const Dereference&)"); }
	void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const InvariantActive&)"); }
	void visit(const Sequence& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const Sequence&)"); }
	void visit(const Scope& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const Scope&)"); }
	void visit(const Atomic& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const Atomic&)"); }
	void visit(const IfThenElse& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const IfThenElse&)"); }
	void visit(const While& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const While&)"); }
	void visit(const Skip& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const Skip&)"); }
	void visit(const Break& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const Break&)"); }
	void visit(const Continue& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const Continue&)"); }
	void visit(const Macro& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const Macro&)"); }
	void visit(const CompareAndSwap& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const CompareAndSwap&)"); }
	void visit(const Function& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) override { throw std::logic_error("Unexpected invocation: RightMovernessVisitor::visit(const Program&)"); }
	void visit(const Choice& /*node*/) override { throw RefinementError("Unsupported construct: choice"); }
	void visit(const Loop& /*node*/) override { throw RefinementError("Unsupported construct: loop"); }

	void visit(const Assume& node) override {
		if (!is_expression_local(*node.expr)) {
			this->moves = false;
		}
	}
	void visit(const Assert& /*node*/) override { /* do nothing */ }
	void visit(const Return& /*node*/) override {
		this->moves = false;
	}
	void visit(const Malloc& node) override {
		if (!node.lhs.is_shared) {
			this->moves = false;
		}
	}
	void visit(const Assignment& node) override {
		if (!is_expression_local(*node.lhs) || !is_expression_local(*node.rhs)) {
			this->moves = false;
		}
	}
	void visit(const Enter& node) override {
		conditionally_raise_error<RefinementError>(&node.decl == &this->retire_function, "Unsupported function: " + retire_function.name);
		events.push_back(node);
	}
	void visit(const Exit& node) override {
		conditionally_raise_error<RefinementError>(&node.decl == &this->retire_function, "Unsupported function: " + retire_function.name);
		events.push_back(node);
	}
};

const VariableDeclaration& make_tmp_variable(Function& function) {
	NameCollectorVisitor visitor;
	function.accept(visitor);
	for (std::size_t i = 0;;++i) {
		std::string var_name = "TMP_assume_" + std::to_string(i) + "_";
		if (visitor.names.count(var_name) == 0) {
			function.body->variables.push_back(std::make_unique<VariableDeclaration>(var_name, Type::bool_type(), false));
			return *function.body->variables.back();
		}
	}
	throw std::logic_error("This is bad.");
}

std::unique_ptr<Statement> make_sequence_from_path(std::vector<const Statement*> path) {
	assert(path.size() > 0);
	std::deque<std::unique_ptr<Statement>> copy;
	for (auto stmt : path) {
		const Command* cmd = dynamic_cast<const Command*>(stmt); // TODO: less hacky way
		conditionally_raise_error<RefinementError>(!cmd, "Casting full path to commands failed.");
		copy.push_back(cola::copy(*cmd));
	}
	assert(path.size() == copy.size());
	
	std::unique_ptr<Statement> result = std::move(copy.front());
	copy.pop_front();
	while (!copy.empty()) {
		result = std::make_unique<Sequence>(std::move(result), std::move(copy.front()));
		copy.pop_front();
	}

	return result;
}


void try_fix_local_unsafe_assume(Program& program, const GuaranteeTable& guarantee_table, const UnsafeAssumeError& error) {
	std::cout << "Trying to remove local unsafe assume by moving it to an earlier program location." << std::endl;

	InsertionLocationFinderVisitor visitor(collect_variables(*error.pc.expr), error.pc);
	auto path = visitor.get_path_or_raise(program);
	conditionally_raise_error<RefinementError>(path.size() == 0, "could not find locations to move assume to; cannot recover");

	std::cout << "Promising control flow locations for moving assume statement to: " << std::endl;
	for (const auto& elem : path) {
		std::cout << "   - ";
		cola::print(*elem, std::cout);
	}

	for (auto it = path.begin(); it != path.end(); ++it) {
		const bool no_check = (it+1 == path.end()) && !CHECK_SINGLETON_PATH;
		if (no_check) {
			std::cout << "Moving assume here: ";
			cola::print(**it, std::cout);
			std::cout << "(Did not check move; had no choice anyways)" << std::endl;
		} else {
			std::cout << "Checking location: ";
			cola::print(**it, std::cout);
		}

		// try to insert assertion in path; move assume if possible
		try {
			auto visitor = insert_active_assertion(program, guarantee_table, **it, error.var, true /* insert after */, no_check);
			if (!no_check) {
				std::cout << " ==> inserting assertion here" << std::endl;
			}

			// create tmp var of type bool
			assert(visitor.found_function);
			const VariableDeclaration& tmp = make_tmp_variable(*visitor.found_function);

			// create update to insert
			auto true_assume = std::make_unique<Assume>(cola::copy(*error.pc.expr));
			auto false_assume = std::make_unique<Assume>(make_negated_expression(cola::copy(*error.pc.expr)));
			auto true_assign = std::make_unique<Assignment>(std::make_unique<VariableExpression>(tmp), std::make_unique<BooleanValue>(true));
			auto false_assign = std::make_unique<Assignment>(std::make_unique<VariableExpression>(tmp), std::make_unique<BooleanValue>(false));
			auto true_branch = std::make_unique<Scope>(std::make_unique<Sequence>(std::move(true_assume), std::move(true_assign)));
			auto false_branch = std::make_unique<Scope>(std::make_unique<Sequence>(std::move(false_assume), std::move(false_assign)));
			auto choice = std::make_unique<Choice>();
			choice->branches.push_back(std::move(true_branch));
			choice->branches.push_back(std::move(false_branch));

			// create new assume to insert
			auto assume = std::make_unique<Assume>(std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, std::make_unique<VariableExpression>(tmp), std::make_unique<BooleanValue>(true)));

			// add new assume, replace old assume with an assertion with the same condition (to maintain type information gained by assume)
			AssertionInsertionVisitor insert_assume_visitor(error.pc, std::move(assume), false /* insert before */);
			program.accept(insert_assume_visitor);
			assert(insert_assume_visitor.owner_found);
			*insert_assume_visitor.owner_found = std::make_unique<Assert>(std::make_unique<InvariantExpression>(cola::copy(*error.pc.expr)));

			// insert update of tmp
			AssertionInsertionVisitor insert_update_visitor(*static_cast<Command*>(visitor.inserted), std::move(choice), true);
			program.accept(insert_update_visitor);

			// done
			return;

		} catch (RefinementError err) {
			std::cout << " ==> cannot insert assertion here" << std::endl;
			// do nothing, continue with next element on path
		}
	}

	raise_error<RefinementError>("could not infer valid move to fix pointer race");
}

std::string expr_to_string(const Expression& expr) {
	std::stringstream result;
	cola::print(expr, result);
	return result.str();
}

void try_remove_unsafe_assume(Program& program, const GuaranteeTable& guarantee_table, const UnsafeAssumeError& error) {
	std::cout << "Trying to remove unsafe assume statement by repeating commands." << std::endl;

	InsertionLocationFinderVisitor visitor(collect_variables(*error.pc.expr), error.pc);
	auto full_path = visitor.get_full_path_or_raise(program);
	conditionally_raise_error<RefinementError>(full_path.size() == 0, "could not find commands to be repeated");

	std::cout << "Commands to be repeated: " << std::endl;
	for (const auto& elem : full_path) {
		std::cout << "   - ";
		cola::print(*elem, std::cout);
	}

	auto it = full_path.begin();
	assert(it != full_path.end());

	// check first path element to be an assignment corresponding to the assumption to be replaced
	auto assignment = dynamic_cast<const Assignment*>(*it);
	conditionally_raise_error<RefinementError>(!assignment, "repeating commands failed (first entry of 'full_path' malformed)");
	auto assumption = dynamic_cast<const BinaryExpression*>(error.pc.expr.get());
	conditionally_raise_error<RefinementError>(!assumption, "repeating commands failed (assume in 'full_path' malformed)");
	auto assignment_lhs = dynamic_cast<const VariableExpression*>(assignment->lhs.get());
	conditionally_raise_error<RefinementError>(!assignment_lhs, "repeating commands failed ('lhs' of assignment in 'full_path' malformed)");
	// auto assumption_lhs = dynamic_cast<const VariableExpression*>(assumption->lhs.get());
	// conditionally_raise_error<RefinementError>(!assumption_lhs, "repeating commands failed ('lhs' of assume in 'full_path' malformed)");
	// auto assignment_rhs = dynamic_cast<const VariableExpression*>(assignment->rhs.get());
	// conditionally_raise_error<RefinementError>(!assignment_rhs, "repeating commands failed ('rhs' of assignment in 'full_path' malformed)");
	// auto assumption_rhs = dynamic_cast<const VariableExpression*>(assumption->rhs.get());
	// conditionally_raise_error<RefinementError>(!assumption_rhs, "repeating commands failed ('rhs' of assume in 'full_path' malformed)");
	// bool well_formed = &assignment_lhs->decl == &assumption_lhs->decl && &assignment_rhs->decl == &assumption_rhs->decl;
	// well_formed |= &assignment_rhs->decl == &assumption_lhs->decl && &assignment_lhs->decl == &assumption_rhs->decl;
	// conditionally_raise_error<RefinementError>(!well_formed, "repeating commands failed ('full_path' malformed)");
	std::string assign_lhs = expr_to_string(*assignment->lhs);
	std::string assign_rhs = expr_to_string(*assignment->rhs);
	std::string assume_lhs = expr_to_string(*assumption->lhs);
	std::string assume_rhs = expr_to_string(*assumption->rhs);
	bool well_formed = assign_lhs == assume_lhs && assume_rhs == assume_rhs;
	well_formed |= assign_lhs == assume_rhs && assume_rhs == assume_lhs;
	conditionally_raise_error<RefinementError>(!well_formed, "repeating commands failed ('full_path' malformed)");
	++it;

	// check remaining path elements to be heap-local or right movers
	RightMovernessVisitor move_checker(guarantee_table.observer_store.retire_function);
	for (; it != full_path.end(); ++it) {
		(*it)->accept(move_checker);
		if (!move_checker.right_moves(guarantee_table.observer_store.simulation)) {
			std::cout << " ==> cannot repeat command: ";
			cola::print(**it, std::cout);
			raise_error<RefinementError>("repeating commands failed (some statements in 'full_path' do not move)");
		}
	}
	assert(!move_checker.right_moves(guarantee_table.observer_store.simulation));
	// conditionally_raise_error<RefinementError>(!move_checker.right_moves(guarantee_table.observer_store.simulation), "repeating commands failed (some statements in 'full_path' do not move)");

	// copy full path and add after offending assume, add active assertion for offending variable
	auto new_code = make_sequence_from_path(full_path);
	AssertionInsertionVisitor insertion(error.pc, std::move(new_code), true /* insert after */);
	program.accept(insertion);

	// delete offending assume 
	*insertion.owner_found = std::make_unique<Skip>();
	std::cout << " ==> replaced unsafe assume with a repetition of the commands" << std::endl;
}

void try_fix_nonlocal_unsafe_assume(Program& program, const GuaranteeTable& guarantee_table, const UnsafeAssumeError& error) {
	std::cout << "Trying to fix unsafe assume statement by inserting an appropriate assertion earlier in the program." << std::endl;
	std::cout << "(Beware, this might not fix the problem but I cannot check this)" << std::endl;

	InsertionLocationFinderVisitor visitor(collect_variables(*error.pc.expr), error.pc);
	auto path = visitor.get_path_or_raise(program);
	conditionally_raise_error<RefinementError>(path.size() == 0, "could not find locations to insert assertions; cannot recover");

	std::cout << "Promising control flow locations for assertion insertion: " << std::endl;
	for (const auto& elem : path) {
		std::cout << "   - ";
		cola::print(*elem, std::cout);
	}

	for (auto it = path.begin(); it != path.end(); ++it) {
		const bool no_check = (it+1 == path.end()) && !CHECK_SINGLETON_PATH;
		if (no_check) {
			std::cout << "Inserting assertion here: ";
			cola::print(**it, std::cout);
			std::cout << "(Did not check insertion; had no choice anyways)" << std::endl;
		} else {
			std::cout << "Checking location: ";
			cola::print(**it, std::cout);
		}

		// try to insert assertion in path; move assume if possible
		try {
			insert_active_assertion(program, guarantee_table, **it, error.var, true /* insert after */, no_check);
			if (!no_check) {
				std::cout << " ==> inserting assertion here" << std::endl;
			}
			return;

		} catch (RefinementError err) {
			std::cout << " ==> cannot insert assertion here: ";
			cola::print(**it, std::cout);
			// do nothing, continue with next element on path
		}
	}

	raise_error<RefinementError>("could not infer valid move to try fix pointer race");
}

void try_fix_local_unsafe_dereference(Program& program, const GuaranteeTable& guarantee_table, const UnsafeDereferenceError& error) {
	std::cout << "Trying to fix unsafe dereference of local pointer by inserting an appropriate assertion earlier in the program." << std::endl;

	InsertionLocationFinderVisitor visitor({ &error.var }, error.pc);
	auto path = visitor.get_path_or_raise(program);
	conditionally_raise_error<RefinementError>(path.size() == 0, "could not find locations to insert assertions; cannot recover");

	std::cout << "Promising control flow locations for assertion insertion: " << std::endl;
	for (const auto& elem : path) {
		std::cout << "   - ";
		cola::print(*elem, std::cout);
	}

	for (auto it = path.begin(); it != path.end(); ++it) {
		const bool no_check = (it+1 == path.end()) && !CHECK_SINGLETON_PATH;
		if (no_check) {
			std::cout << "Inserting assertion here: ";
			cola::print(**it, std::cout);
			std::cout << "(Did not check insertion; had no choice anyways)" << std::endl;
		} else {
			std::cout << "Checking location: ";
			cola::print(**it, std::cout);
		}

		// try to insert assertion in path; move assume if possible
		try {
			insert_active_assertion(program, guarantee_table, **it, error.var, true /* insert after */, no_check);
			if (!no_check) {
				std::cout << " ==> inserting assertion here" << std::endl;
			}
			return;

		} catch (RefinementError err) {
			std::cout << " ==> cannot insert assertion here: ";
			cola::print(**it, std::cout);
			// do nothing, continue with next element on path
		}
	}

	raise_error<RefinementError>("could not infer valid move to fix pointer race");
}


void prtypes::try_fix_pointer_race(Program& program, const GuaranteeTable& guarantee_table, const UnsafeAssumeError& error, bool avoid_reoffending) {
	try {
		// insert assertion for offending command
		insert_active_assertion(program, guarantee_table, error.pc, error.var);
		std::cout << " ==> inserted active assertion for variable '" << error.var.name << "'" << std::endl;

	} catch (RefinementError err) {
		std::cout << "Variable '" << error.var.name << "' is not active in: ";
		cola::print(error.pc, std::cout);
		std::cout << "(Cannot insert assertion to fix pointer race)" << std::endl;

		// try to move the offending assertion
		assert(error.pc.expr);
		if (is_expression_local(*error.pc.expr)) {
			try_fix_local_unsafe_assume(program, guarantee_table, error);
		} else {
			try {
				try_remove_unsafe_assume(program, guarantee_table, error);
			} catch (RefinementError suberr) {
				if (avoid_reoffending) {
					throw suberr;
				} else {
					std::cout << suberr.what() << std::endl;
					try_fix_nonlocal_unsafe_assume(program, guarantee_table, error);
				}
			}
		}

	}
}

void prtypes::try_fix_pointer_race(cola::Program& program, const GuaranteeTable& guarantee_table, const UnsafeCallError& error) {
	if (&error.pc.decl == &guarantee_table.observer_store.retire_function) {
		assert(error.pc.args.size() == 1);
		assert(error.pc.args.at(0));
		const VariableExpression& expr = *static_cast<const VariableExpression*>(error.pc.args.at(0).get()); // TODO: unhack this
		insert_active_assertion(program, guarantee_table, error.pc, expr.decl);
		std::cout << " ==> inserted active assertion for variable '" << expr.decl.name << "'" << std::endl;

	} else {
		raise_error<RefinementError>("cannot recover from unsafe call to function " + error.pc.decl.name);
	}
}

void prtypes::try_fix_pointer_race(cola::Program& program, const GuaranteeTable& guarantee_table, const UnsafeDereferenceError& error) {
	try {
		// insert assertion for offending command
		insert_active_assertion(program, guarantee_table, error.pc, error.var);
		std::cout << " ==> inserted active assertion for variable '" << error.var.name << "'" << std::endl;

	} catch (RefinementError err) {
		std::cout << "Variable '" << error.var.name << "' is not active in: ";
		cola::print(error.pc, std::cout);
		std::cout << "(Cannot insert assertion to fix pointer race)" << std::endl;

		if (!error.var.is_shared) {
			// dereferences of local pointers should be guarded by SMR; try find earlier point where it is safe
			try_fix_local_unsafe_dereference(program, guarantee_table, error);
		} else {
			throw err;
		}

	}
}
