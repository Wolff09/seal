#include "types/preprocess.hpp"
#include "cola/util.hpp"
#include "cola/transform.hpp"
#include "types/error.hpp"
#include <iostream>
#include <set>

using namespace cola;
using namespace prtypes;


struct MacroCopyVisitor final : public Visitor {
	const Macro& to_copy;
	std::map<const VariableDeclaration*, const VariableDeclaration*> intern2extern;
	std::unique_ptr<AstNode> result;

	MacroCopyVisitor(const Macro& to_copy) : to_copy(to_copy) {
		conditionally_raise_error<UnsupportedConstructError>(to_copy.decl.return_type != Type::void_type(), "inline functions must have void type");
		assert(to_copy.args.size() == to_copy.decl.args.size());
		for (std::size_t i = 0; i < to_copy.args.size(); ++i) {
			const Expression* arg = to_copy.args.at(i).get();
			conditionally_raise_error<UnsupportedConstructError>(typeid(*arg) != typeid(VariableExpression), "only variables may be passed to inline functions");
			const VariableDeclaration* external_decl = &static_cast<const VariableExpression*>(arg)->decl;
			const VariableDeclaration* internal_decl = to_copy.decl.args.at(i).get();
			conditionally_raise_error<UnsupportedConstructError>(external_decl->type != internal_decl->type, "type missmatch in invocation of inline function '" + to_copy.decl.name + "'");
			intern2extern.insert({ internal_decl, external_decl });
		}
	}

	template<typename R>
	std::unique_ptr<R> copy_node(const AstNode& node) {
		{
			const AnnotatedStatement* tmp = dynamic_cast<const AnnotatedStatement*>(&node);
			conditionally_raise_error<UnsupportedConstructError>(tmp && tmp->annotation, "annotations are not supported in inline functions");
		}
		node.accept(*this);
		AstNode* res_ptr = result.release();
		R* res_cast = dynamic_cast<R*>(res_ptr);
		if (!res_cast) {
			throw std::logic_error("I got lost casting...");
		}
		std::unique_ptr<R> result;
		result.reset(res_cast);
		return result;
	}

	void visit(const Function& /*node*/) { throw std::logic_error("Unexpected invocation: MacroCopyVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) { throw std::logic_error("Unexpected invocation: MacroCopyVisitor::visit(const Program&)"); }

	void visit(const VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation: MacroCopyVisitor::visit(const VariableDeclaration&)"); }
	void visit(const Expression& /*node*/) { throw std::logic_error("Unexpected invocation: MacroCopyVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& node) {
		result = std::make_unique<BooleanValue>(node.value);
	}
	void visit(const NullValue& /*node*/) {
		result = std::make_unique<NullValue>();
	}
	void visit(const EmptyValue& /*node*/) {
		result = std::make_unique<EmptyValue>();
	}
	void visit(const MaxValue& /*node*/) {
		result = std::make_unique<MaxValue>();
	}
	void visit(const MinValue& /*node*/) {
		result = std::make_unique<MinValue>();
	}
	void visit(const NDetValue& /*node*/) {
		result = std::make_unique<NDetValue>();
	}
	void visit(const VariableExpression& node) {
		const VariableDeclaration* decl;
		if (node.decl.is_shared) {
			decl = &node.decl;
		} else {
			auto search = intern2extern.find(&node.decl);
			conditionally_raise_error<std::logic_error>(search == intern2extern.end(), "Unexpected variable '" + node.decl.name + "'; could not externatlize.");
			decl = search->second;
		}
		assert(decl);
		result = std::make_unique<VariableExpression>(*decl);
	}
	void visit(const NegatedExpression& node) {
		result = std::make_unique<NegatedExpression>(copy_node<Expression>(*node.expr));
	}
	void visit(const BinaryExpression& node) {
		result = std::make_unique<BinaryExpression>(node.op, copy_node<Expression>(*node.lhs), copy_node<Expression>(*node.rhs));
	}
	void visit(const Dereference& node) {
		result = std::make_unique<Dereference>(copy_node<Expression>(*node.expr), node.fieldname);
	}
	void visit(const InvariantExpression& node) {
		result = std::make_unique<InvariantExpression>(copy_node<Expression>(*node.expr));
	}
	void visit(const InvariantActive& node) {
		result = std::make_unique<InvariantActive>(copy_node<Expression>(*node.expr));
	}

	void visit(const Sequence& node) {
		result = std::make_unique<Sequence>(copy_node<Statement>(*node.first), copy_node<Statement>(*node.second));
	}
	void visit(const Scope& node) {
		conditionally_raise_error<UnsupportedConstructError>(!node.variables.empty(), "Cannot copy variables from inline function");
		result = std::make_unique<Scope>(copy_node<Statement>(*node.body));
	}
	void visit(const Atomic& node) {
		result = std::make_unique<Atomic>(copy_node<Scope>(*node.body));
	}
	void visit(const Choice& node) {
		auto stmt = std::make_unique<Choice>();
		for (const auto& branch : node.branches) {
			stmt->branches.push_back(copy_node<Scope>(*branch));
		}
		result = std::move(stmt);
	}
	void visit(const IfThenElse& node) {
		result = std::make_unique<IfThenElse>(copy_node<Expression>(*node.expr), copy_node<Scope>(*node.ifBranch), copy_node<Scope>(*node.elseBranch));
	}
	void visit(const Loop& node) {
		result = std::make_unique<Loop>(copy_node<Scope>(*node.body));
	}
	void visit(const While& node) {
		result = std::make_unique<While>(copy_node<Expression>(*node.expr), copy_node<Scope>(*node.body));
	}
	void visit(const Skip& /*node*/) {
		result = std::make_unique<Skip>();
	}
	void visit(const Break& /*node*/) {
		result = std::make_unique<Break>();
	}
	void visit(const Continue& /*node*/) {
		result = std::make_unique<Continue>();
	}
	void visit(const Assume& node) {
		result = std::make_unique<Assume>(copy_node<Expression>(*node.expr));
	}
	void visit(const Assert& node) {
		result = std::make_unique<Assert>(copy_node<Invariant>(*node.inv));
	}
	void visit(const AngelChoose& /*node*/) {
		raise_error<UnsupportedConstructError>("angels are not supported in inline functions");
	}
	void visit(const AngelActive& /*node*/) {
		raise_error<UnsupportedConstructError>("angels are not supported in inline functions");
	}
	void visit(const AngelContains& /*node*/) {
		raise_error<UnsupportedConstructError>("angels are not supported in inline functions");
	}
	void visit(const Return& /*node*/) {
		raise_error<UnsupportedConstructError>("'return' is not supported in inline functions");
	}
	void visit(const Malloc& /*node*/) {
		raise_error<UnsupportedConstructError>("'malloc' is not supported in inline functions");
	}
	void visit(const Assignment& node) {
		result = std::make_unique<Assignment>(copy_node<Expression>(*node.lhs), copy_node<Expression>(*node.rhs));
	}
	void visit(const Enter& node) {
		auto stmt = std::make_unique<Enter>(node.decl);
		for (auto const& arg : node.args) {
			stmt->args.push_back(copy_node<Expression>(*arg));
		}
		result = std::move(stmt);
	}
	void visit(const Exit& node) {
		result = std::make_unique<Exit>(node.decl);
	}
	void visit(const Macro& /*node*/) {
		raise_error<UnsupportedConstructError>("inline function calls are not supported in inline functions");
	}
	void visit(const CompareAndSwap& /*node*/) {
		raise_error<UnsupportedConstructError>("'CAS' not supported in inline functions");
	}
};

struct InliningVisitor final : public NonConstVisitor {
	std::unique_ptr<Statement>* owner;

	void visit(VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(VariableDeclaration&)"); }
	void visit(Expression& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(Expression&)"); }
	void visit(BooleanValue& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(BooleanValue&)"); }
	void visit(NullValue& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(NullValue&)"); }
	void visit(EmptyValue& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(EmptyValue&)"); }
	void visit(MaxValue& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(MaxValue&)"); }
	void visit(MinValue& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(MinValue&)"); }
	void visit(NDetValue& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(NDetValue&)"); }
	void visit(VariableExpression& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(VariableExpression&)"); }
	void visit(NegatedExpression& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(NegatedExpression&)"); }
	void visit(BinaryExpression& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(BinaryExpression&)"); }
	void visit(Dereference& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(Dereference&)"); }
	void visit(InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(InvariantExpression&)"); }
	void visit(InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: InliningVisitor::visit(InvariantActive&)"); }

	void visit(Skip& /*node*/) { /* do nothing */ }
	void visit(Break& /*node*/) { /* do nothing */ }
	void visit(Continue& /*node*/) { /* do nothing */ }
	void visit(Assume& /*node*/) { /* do nothing */ }
	void visit(Assert& /*node*/) { /* do nothing */ }
	void visit(AngelChoose& /*node*/) { /* do nothing */ }
	void visit(AngelActive& /*node*/) { /* do nothing */ }
	void visit(AngelContains& /*node*/) { /* do nothing */ }
	void visit(Return& /*node*/) { /* do nothing */ }
	void visit(Malloc& /*node*/) { /* do nothing */ }
	void visit(Assignment& /*node*/) { /* do nothing */ }
	void visit(Enter& /*node*/) { /* do nothing */ }
	void visit(Exit& /*node*/) { /* do nothing */ }
	void visit(CompareAndSwap& /*node*/) { /* do nothing */ }

	void visit(Macro& node) {
		assert(owner);
		MacroCopyVisitor visitor(node);
		*owner = visitor.copy_node<Scope>(*node.decl.body);
	}

	void handle_statement(std::unique_ptr<Statement>& stmt) {
		owner = &stmt;
		stmt->accept(*this);
	}
	void visit(Sequence& node) {
		handle_statement(node.first);
		handle_statement(node.second);
	}
	void visit(Scope& node) {
		handle_statement(node.body);
	}
	void visit(Atomic& node) {
		node.body->accept(*this);
	}
	void visit(Choice& node) {
		for (const auto& branch : node.branches) {
			branch->accept(*this);
		}
	}
	void visit(IfThenElse& node) {
		node.ifBranch->accept(*this);
		node.elseBranch->accept(*this);
	}
	void visit(Loop& node) {
		node.body->accept(*this);
	}
	void visit(While& node) {
		node.body->accept(*this);
	}
	void visit(Function& function) {
		if (function.kind == Function::INTERFACE && function.body) {
			function.body->accept(*this);
		}
	}
	void visit(Program& program) {
		for (const auto& function : program.functions) {
			function->accept(*this);
		}

		auto functions = std::move(program.functions);
		for (auto& function : functions) {
			if (function->kind != Function::MACRO) {
				program.functions.push_back(std::move(function));
			}
		}
	}
};

struct CollectDerefVisitor final : public Visitor {
	std::vector<const Dereference*> derefs;
	bool is_assume = true;

	void visit(const VariableDeclaration& /*node*/) { /* do nothing */ }
	void visit(const BooleanValue& /*node*/) { /* do nothing */ }
	void visit(const NullValue& /*node*/) { /* do nothing */ }
	void visit(const EmptyValue& /*node*/) { /* do nothing */ }
	void visit(const MaxValue& /*node*/) { /* do nothing */ }
	void visit(const MinValue& /*node*/) { /* do nothing */ }
	void visit(const NDetValue& /*node*/) { /* do nothing */ }
	void visit(const VariableExpression& /*node*/) { /* do nothing */ }
	void visit(const NegatedExpression& node) {
		assert(node.expr);
		node.expr->accept(*this);
	}
	void visit(const BinaryExpression& node) {
		assert(node.lhs);
		node.lhs->accept(*this);
		assert(node.rhs);
		node.rhs->accept(*this);
	}
	void visit(const Dereference& node) {
		if (node.sort() == Sort::PTR) {
			this->derefs.push_back(&node);
		}
	}

	void visit(const Assume& node) {
		assert(node.expr);
		node.expr->accept(*this);
	}
	
	void visit(const Expression& /*node*/) { throw std::logic_error("Unexpected invocation: CollectDerefVisitor::visit(const Expression&)"); }
	void visit(const InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: CollectDerefVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: CollectDerefVisitor::visit(const InvariantActive&)"); }
	void visit(const Sequence& /*node*/) { is_assume = false; }
	void visit(const Scope& /*node*/) { is_assume = false; }
	void visit(const Atomic& /*node*/) { is_assume = false; }
	void visit(const Choice& /*node*/) { is_assume = false; }
	void visit(const IfThenElse& /*node*/) { is_assume = false; }
	void visit(const Loop& /*node*/) { is_assume = false; }
	void visit(const While& /*node*/) { is_assume = false; }
	void visit(const Skip& /*node*/) { is_assume = false; }
	void visit(const Break& /*node*/) { is_assume = false; }
	void visit(const Continue& /*node*/) { is_assume = false; }
	void visit(const Assert& /*node*/) { is_assume = false; }
	void visit(const AngelChoose& /*node*/) { is_assume = false; }
	void visit(const AngelActive& /*node*/) { is_assume = false; }
	void visit(const AngelContains& /*node*/) { is_assume = false; }
	void visit(const Return& /*node*/) { is_assume = false; }
	void visit(const Malloc& /*node*/) { is_assume = false; }
	void visit(const Assignment& /*node*/) { is_assume = false; }
	void visit(const Enter& /*node*/) { is_assume = false; }
	void visit(const Exit& /*node*/) { is_assume = false; }
	void visit(const Macro& /*node*/) { is_assume = false; }
	void visit(const CompareAndSwap& /*node*/) { is_assume = false; }
	void visit(const Function& /*node*/) { throw std::logic_error("Unexpected invocation: CollectDerefVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) { throw std::logic_error("Unexpected invocation: CollectDerefVisitor::visit(const Program&)"); }
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
	void visit(const AngelChoose& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const AngelChoose&)"); }
	void visit(const AngelActive& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const AngelActive&)"); }
	void visit(const AngelContains& /*node*/) { throw std::logic_error("Unexpected invocation: LocalExpressionVisitor::visit(const AngelContains&)"); }
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

inline bool is_expression_local(const Expression& expr) {
	LocalExpressionVisitor visitor;
	expr.accept(visitor);
	return visitor.result;
}

struct NeedsAtomicVisitor final : public Visitor {
	bool result = true;
	const Function& retire_function;
	NeedsAtomicVisitor(const Function& retire_function) : retire_function(retire_function) {}

	void visit(const VariableDeclaration& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const VariableDeclaration&)"); }
	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const EmptyValue&)"); }
	void visit(const MaxValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const MaxValue&)"); }
	void visit(const MinValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const MinValue&)"); }
	void visit(const NDetValue& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const NDetValue&)"); }
	void visit(const VariableExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const VariableExpression&)"); }
	void visit(const NegatedExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const BinaryExpression&)"); }
	void visit(const Dereference& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const Dereference&)"); }
	void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const InvariantActive&)"); }
	void visit(const Function& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) override { throw std::logic_error("Unexpected invocation: NeedsAtomicVisitor::visit(const Program&)"); }

	void visit(const Sequence& /*node*/) override { /* do nothing */ }
	void visit(const Scope& /*node*/) override { /* do nothing */ }
	void visit(const Choice& /*node*/) override { /* do nothing */ }
	void visit(const IfThenElse& /*node*/) override { /* do nothing */ }
	void visit(const Loop& /*node*/) override { /* do nothing */ }
	void visit(const While& /*node*/) override { /* do nothing */ }
	void visit(const Return& /*node*/) override { /* do nothing */ }
	void visit(const Macro& /*node*/) override { /* do nothing */ }
	void visit(const CompareAndSwap& /*node*/) override { /* do nothing */ }

	void visit(const Atomic& /*node*/) override { this->result = false; }
	void visit(const Skip& /*node*/) override { this->result = false; }
	void visit(const Break& /*node*/) override { this->result = false; }
	void visit(const Continue& /*node*/) override { this->result = false; }
	void visit(const AngelChoose& /*node*/) override { this->result = false; } // TODO: correct?
	void visit(const AngelActive& /*node*/) override { this->result = false; } // TODO: correct?
	void visit(const AngelContains& /*node*/) override { this->result = false; } // TODO: correct?
	void visit(const Assume& node) override {
		if (is_expression_local(*node.expr)) {
			this->result = false;
		}
	}
	void visit(const Assert& node) override {
		if (is_expression_local(*node.inv->expr)) {
			this->result = false;
		}
	}
	void visit(const Malloc& node) override {
		if (!node.lhs.is_shared) {
			this->result = false;
		}
	}
	void visit(const Assignment& node) override {
		if (is_expression_local(*node.lhs) && is_expression_local(*node.rhs)) {
			this->result = false;
		}
	}
	void visit(const Enter& node) override {
		if (&node.decl == &retire_function) {
			return;
		}
		for (const auto& arg : node.args) {
			if (!is_expression_local(*arg)) {
				return;
			}
		}
		this->result = false;
	}
	void visit(const Exit& /*node*/) override { this->result = false; }
};

bool needs_atomic(const Statement& stmt, const Function& retire_function) {
	NeedsAtomicVisitor visitor(retire_function);
	stmt.accept(visitor);
	return visitor.result;
}

struct PreprocessingVisitor final : public NonConstVisitor {
	bool in_atomic = false;
	bool found_return = false;
	bool found_cmd = false;
	const Function& retire_function;
	PreprocessingVisitor(const Function& retire_function) : retire_function(retire_function) {}

	void visit(VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(VariableDeclaration&)"); }
	void visit(Expression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(Expression&)"); }
	void visit(BooleanValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(BooleanValue&)"); }
	void visit(NullValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(NullValue&)"); }
	void visit(EmptyValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(EmptyValue&)"); }
	void visit(MaxValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(MaxValue&)"); }
	void visit(MinValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(MinValue&)"); }
	void visit(NDetValue& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(NDetValue&)"); }
	void visit(VariableExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(VariableExpression&)"); }
	void visit(NegatedExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(NegatedExpression&)"); }
	void visit(BinaryExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(BinaryExpression&)"); }
	void visit(Dereference& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(Dereference&)"); }
	void visit(InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(InvariantExpression&)"); }
	void visit(InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(InvariantActive&)"); }
	
	void visit(IfThenElse& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(IfThenElse&)"); }
	void visit(While& node) {
		auto& expr = *node.expr;
		assert(typeid(expr) == typeid(BooleanValue));
		node.body->accept(*this);
	}
	void visit(CompareAndSwap& /*node*/) { throw std::logic_error("Unexpected invocation: PreprocessingVisitor::visit(CompareAndSwap&)"); }
	void visit(Continue& /*node*/) { raise_error<UnsupportedConstructError>("'continue' not supported"); }
	void visit(Macro& /*node*/) { raise_error<UnsupportedConstructError>("inline functions not supported"); }

	void handle_assume(std::unique_ptr<Statement>& stmt) {
		assert(stmt);
		CollectDerefVisitor visitor;
		stmt->accept(visitor);
		if (visitor.is_assume && !visitor.derefs.empty()) {
			// cola::print(*stmt, std::cout)
			std::unique_ptr<Statement> result = std::move(stmt);
			for (const auto& deref : visitor.derefs) {
				assert(deref);
				auto assertion = std::make_unique<Assert>(std::make_unique<InvariantActive>(cola::copy(*deref)));
				result = std::make_unique<Sequence>(std::move(result), std::move(assertion));
			}
			stmt = std::move(result);
		}
		assert(stmt);
	}

	void handle_statement(std::unique_ptr<Statement>& stmt) {
		found_cmd = false;
		assert(stmt);
		stmt->accept(*this);
		handle_assume(stmt);
		if (found_cmd && !in_atomic && needs_atomic(*stmt, retire_function)) {
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
		conditionally_raise_error<UnsupportedConstructError>(node.branches.size() == 1, "'choice' with only one branch indicates error; maybe you meant to add a branch containing just 'skip'");
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

	void visit(Break& /*node*/) { found_cmd = true; }
	void visit(Skip& /*node*/) { found_cmd = true; }
	void visit(Assume& /*node*/) { found_cmd = true; }
	void visit(Assert& /*node*/) { found_cmd = true; }
	void visit(AngelChoose& /*node*/) { found_cmd = true; }
	void visit(AngelActive& /*node*/) { found_cmd = true; }
	void visit(AngelContains& /*node*/) { found_cmd = true; }
	void visit(Return& /*node*/) { found_cmd = true; found_return = true; }
	void visit(Malloc& /*node*/) { found_cmd = true; }
	void visit(Assignment& /*node*/) { found_cmd = true; }
	void visit(Enter& /*node*/) { found_cmd = true; }
	void visit(Exit& /*node*/) { found_cmd = true; }

	void visit(Function& node) {
		found_return = false;
		if (node.body) {
			node.body->accept(*this);
		}
	}
	void visit(Program& node) {
		// TODO: handle initializer
		for (auto& function : node.functions) {
			assert(function);
			function->accept(*this);
		}
	}
};

void prtypes::preprocess(Program& program, const cola::Function& retire_function) {
	InliningVisitor inliner;
	program.accept(inliner);

	cola::remove_cas(program);
	cola::remove_scoped_variables(program);
	cola::remove_conditionals(program);
	cola::remove_useless_scopes(program);

	PreprocessingVisitor visitor(retire_function);
	program.accept(visitor);
}
