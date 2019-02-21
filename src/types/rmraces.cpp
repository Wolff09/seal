#include "types/rmraces.hpp"
#include "types/cave.hpp"
#include "cola/util.hpp"
#include <iostream>

using namespace cola;
using namespace prtypes;


std::unique_ptr<Expression> make_true_guard() {
	return std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, std::make_unique<BooleanValue>(true), std::make_unique<BooleanValue>(true));
}

struct NameCollectorVisitor final : public Visitor {
	std::set<std::string> names;
	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NameCollectorVisitor::visit(const EmptyValue&)"); }
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
	std::unique_ptr<Statement> to_insert;
	const bool insert_after;
	Statement* insertion_parent = nullptr; // may come in handy to undo insertion
	Function* current_function;
	Function* found_function;
	Statement* inserted;
	Statement* found;
	AssertionInsertionVisitor(const Command& to_find, std::unique_ptr<Statement> to_insert, bool insert_after) : to_find(to_find), to_insert(std::move(to_insert)), insert_after(insert_after) {}

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
		this->inserted = this->to_insert.get();
		this->found = found.get();
		assert(this->found);
		std::unique_ptr<Sequence> result;
		if (this->insert_after) {
			// insert: to_find - to_insert
			result = std::make_unique<Sequence>(std::move(found), std::move(this->to_insert));
		} else {
			// insert: to_insert - to_find
			result = std::make_unique<Sequence>(std::move(this->to_insert), std::move(found));
		}
		assert(result);
		this->insertion_parent = result.get();
		this->found_function = this->current_function;
		assert(!this->to_insert);
		assert(this->insertion_parent);
		assert(this->found_function);
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

AssertionInsertionVisitor insert_active_assertion(Program& program, const GuaranteeTable& guarantee_table, const Command& cmd, const VariableDeclaration& var, bool insert_after_cmd=false) {
	// make insertion to insert
	auto assertion = std::make_unique<Assert>(std::make_unique<InvariantActive>(std::make_unique<VariableExpression>(var)));
	Assert& inserted_assertion = *assertion.get();

	// patch program
	AssertionInsertionVisitor visitor(cmd, std::move(assertion), insert_after_cmd);
	program.accept(visitor);
	assert(!visitor.to_insert);

	// check added assertion for validity
	bool is_inserted_assertion_valid = prtypes::discharge_assertions(program, guarantee_table, { inserted_assertion });
	if (!is_inserted_assertion_valid) {
		// roll back insertion, then fail
		inserted_assertion.inv = std::make_unique<InvariantExpression>(make_true_guard());
		raise_error<RefinementError>("could not infer valid assertion to fix pointer race");
	}
	return visitor;
}

struct AssignmentExpressionVisitor final : public Visitor {
	const VariableDeclaration& search;
	bool result = false;
	AssignmentExpressionVisitor(const VariableDeclaration& search) : search(search) {}

	void visit(const VariableDeclaration& decl) override {
		if (&decl == &search) {
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
	const VariableDeclaration& variable;
	const Command& end;
	bool on_path = false;
	std::vector<const Command*> path;
	InsertionLocationFinderVisitor(const VariableDeclaration& variable, const Command& end) : variable(variable), end(end) {
		assert(!variable.is_shared);
	}

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

	void abort() {
		throw Heureka();
	}

	void visit(const VariableDeclaration& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const VariableDeclaration&)"); }
	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const EmptyValue&)"); }
	void visit(const NDetValue& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const NDetValue&)"); }
	void visit(const VariableExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const VariableExpression&)"); }
	void visit(const NegatedExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const BinaryExpression&)"); }
	void visit(const Dereference& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const Dereference&)"); }
	void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: InsertionLocationFinderVisitor::visit(const InvariantActive&)"); }

	// assignment to variable => on_path = true, path.clear()
	void visit(const Skip& /*node*/) override { /* do nothing */ }
	void visit(const Break& /*node*/) override { /* do nothing */ }
	void visit(const Continue& /*node*/) override { /* do nothing */ }
	void visit(const Return& /*node*/) override { /* do nothing */ }
	void visit(const Enter& /*node*/) override { /* do nothing */ }
	void visit(const Exit& /*node*/) override { /* do nothing */ }
	void visit(const Macro& /*node*/) override { /* do nothing */ }
	void visit(const CompareAndSwap& /*node*/) override { /* do nothing */ }
	void visit(const Assert& /*node*/) override { /* do nothing */ }
	void visit(const Malloc& malloc) override {
		if (&malloc.lhs == &variable) {
			on_path = true;
			path.clear();
			path.push_back(&malloc);
		}
	}
	void visit(const Assignment& assign) override {
		AssignmentExpressionVisitor visitor(variable);
		assert(assign.lhs);
		assign.lhs->accept(visitor);

		if (visitor.result) {
			// assignment to this->variable
			on_path = true;
			path.clear();
			path.push_back(&assign);
		} 
	}
	void visit(const Assume& node) override {
		assert(node.expr);
		if (on_path && !is_expression_local(*node.expr)) {
			path.push_back(&node);
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
		bool copy_on = on_path;
		for (const auto& branch : node.branches) {
			assert(branch);
			branch->accept(*this);

			path = std::move(copy_path);
			on_path = copy_on;
		}
	}
	void visit(const Loop& node) override {
		std::vector<const Command*> copy_path = path;
		bool copy_on = on_path;
		
		assert(node.body);
		node.body->accept(*this);

		path = std::move(copy_path);
		on_path = copy_on;
	}
	void visit(const IfThenElse& /*node*/) override {
		raise_error<UnsupportedConstructError>("'if' not supported");
	}
	void visit(const While& /*node*/) override {
		raise_error<UnsupportedConstructError>("'while' not supported");
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


void try_fix_local_unsafe_assume(Program& program, const GuaranteeTable& guarantee_table, const UnsafeAssumeError& error) {
	// TODO: check if can move; if so, move and return
	// TODO: find a list of previous assume/assert statements, try to add an assert(active(...)) there, if possible then do the check there and store the result in a variable (also propagate this check to other assertions)
	
	std::cout << "Trying to fix local unsafe assume..." << std::endl;
	cola::print(error.pc, std::cout);

	InsertionLocationFinderVisitor visitor(error.var, error.pc); // TODO: the path must be valid for all variables occuring in the assume expr
	auto path_opt = visitor.get_path(program);
	conditionally_raise_error<RefinementError>(!path_opt.has_value(), "could not find an insertion path for unsafe assume");
	auto path = *path_opt;

	std::cout << "Extracted insertion path: " << std::endl;
	for (const auto& cmd : path) {
		std::cout << "   - ";
		cola::print(*cmd, std::cout);
	}

	for (auto it = path.rbegin(); it != path.rend(); ++it) {
		// try to insert assertion in path; move assume if possible
		std::cout << "Handling: ";
		cola::print(**it, std::cout);
		try {
			auto visitor = insert_active_assertion(program, guarantee_table, **it, error.var, true /* insert after */);
			std::cout << "** Found move destination! **" << std::endl;
			std::cout << "  to_find: "; cola::print(visitor.to_find, std::cout);
			std::cout << "  insert_after: " << visitor.insert_after << std::endl;
			// std::cout << "  insertion_parent: "; cola::print(*visitor.insertion_parent, std::cout);
			std::cout << "  inserted: "; cola::print(*static_cast<Command*>(visitor.inserted), std::cout);
			std::cout << "  found: "; cola::print(*static_cast<Command*>(visitor.found), std::cout);
			std::cout << "  found_function: " << visitor.found_function->name << std::endl;
			std::cout << "  problematic assume: "; cola::print(error.pc, std::cout);

			// create tmp var of type bool
			assert(visitor.found_function);
			const VariableDeclaration& tmp = make_tmp_variable(*visitor.found_function);

			// create update to insert
			auto true_assume = std::make_unique<Assume>(cola::copy(*error.pc.expr));
			auto false_assume = std::make_unique<Assume>(std::make_unique<NegatedExpression>(cola::copy(*error.pc.expr)));
			auto true_assign = std::make_unique<Assignment>(std::make_unique<VariableExpression>(tmp), std::make_unique<BooleanValue>(true));
			auto false_assign = std::make_unique<Assignment>(std::make_unique<VariableExpression>(tmp), std::make_unique<BooleanValue>(false));
			auto true_branch = std::make_unique<Scope>(std::make_unique<Sequence>(std::move(true_assume), std::move(true_assign)));
			auto false_branch = std::make_unique<Scope>(std::make_unique<Sequence>(std::move(false_assume), std::move(false_assign)));
			auto choice = std::make_unique<Choice>();
			choice->branches.push_back(std::move(true_branch));
			choice->branches.push_back(std::move(false_branch));

			// create new assume to insert
			auto assume = std::make_unique<Assume>(std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, std::make_unique<VariableExpression>(tmp), std::make_unique<BooleanValue>(true)));

			// add new assume, replace old assume condition with a tautology
			AssertionInsertionVisitor insert_assume_visitor(error.pc, std::move(assume), true);
			program.accept(insert_assume_visitor);
			static_cast<Assume*>(insert_assume_visitor.found)->expr = make_true_guard();

			// insert update of tmp
			AssertionInsertionVisitor insert_update_visitor(*static_cast<Command*>(visitor.inserted), std::move(choice), true);
			program.accept(insert_update_visitor);

			std::cout << std::endl << std::endl << std::endl << std::endl;
			std::cout << "###############################################" << std::endl;
			std::cout << "################### updated ###################" << std::endl;
			std::cout << "###############################################" << std::endl;
			cola::print(program, std::cout);

			// TODO: check if all (added) assertions hold now?
			throw std::logic_error("not yet implemented: moving local assume");
			return;

		} catch (RefinementError err) {
			// TODO: do nothing, continue with next element on path
		}
	}
	
	raise_error<RefinementError>("could not infer valid move to fix pointer race");
}

void prtypes::try_fix_pointer_race(Program& program, const GuaranteeTable& guarantee_table, const UnsafeAssumeError& error) {
	try {
		// insert assertion for offending command
		insert_active_assertion(program, guarantee_table, error.pc, error.var);

	} catch (RefinementError err) {
		// try to move the offending assertion
		assert(error.pc.expr);
		if (is_expression_local(*error.pc.expr)) {
			try_fix_local_unsafe_assume(program, guarantee_table, error);
		} else {
			throw err;
		}

	}
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







