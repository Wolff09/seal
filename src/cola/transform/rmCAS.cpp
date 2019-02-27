#include "cola/transform.hpp"
#include "cola/util.hpp"
#include <set>

using namespace cola;


struct CasFinderVisitor : public Visitor {
	const CompareAndSwap* found = nullptr;
	bool on_top_level = true;
	bool found_low_level = false;

	void visit(const VariableDeclaration& /*node*/) {
		on_top_level = false;
	}
	void visit(const BooleanValue& /*node*/) {
		on_top_level = false;
	}
	void visit(const NullValue& /*node*/) {
		on_top_level = false;
	}
	void visit(const EmptyValue& /*node*/) {
		on_top_level = false;
	}
	void visit(const NDetValue& /*node*/) {
		on_top_level = false;
	}
	void visit(const VariableExpression& /*node*/) {
		on_top_level = false;
	}
	void visit(const NegatedExpression& node) {
		on_top_level = false;
		node.expr->accept(*this);
	}
	void visit(const BinaryExpression& node) {
		on_top_level = false;
		node.lhs->accept(*this);
		node.rhs->accept(*this);
	}
	void visit(const Dereference& node) {
		on_top_level = false;
		node.expr->accept(*this);
	}
	void visit(const CompareAndSwap& node) {
		if (!this->on_top_level) {
			this->found_low_level = true;
		}
		found = &node;
	}
	
	void visit(const Expression& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Expression&)"); }
	void visit(const InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const InvariantActive&)"); }
	void visit(const Sequence& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Sequence&)"); }
	void visit(const Scope& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Scope&)"); }
	void visit(const Atomic& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Atomic&)"); }
	void visit(const Choice& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Choice&)"); }
	void visit(const IfThenElse& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const IfThenElse&)"); }
	void visit(const Loop& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Loop&)"); }
	void visit(const While& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const While&)"); }
	void visit(const Skip& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Skip&)"); }
	void visit(const Break& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Break&)"); }
	void visit(const Continue& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Continue&)"); }
	void visit(const Assume& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Assume&)"); }
	void visit(const Assert& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Assert&)"); }
	void visit(const Return& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Return&)"); }
	void visit(const Malloc& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Malloc&)"); }
	void visit(const Assignment& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Assignment&)"); }
	void visit(const Enter& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Enter&)"); }
	void visit(const Exit& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Exit&)"); }
	void visit(const Macro& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Macro&)"); }
	void visit(const Function& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) { throw std::logic_error("Unexpected invocation: CasFinderVisitor::visit(const Program&)"); }
};

bool contains_cas(const Expression& expr) {
	CasFinderVisitor visitor;
	expr.accept(visitor);
	return visitor.found != nullptr;
}

VariableDeclaration& make_tmp_var(Function& function) {
	std::set<std::string> names;
	for (const auto& var : function.body->variables) {
		names.insert(var->name);
	}
	std::size_t counter = 0;
	while (true) {
		std::string var_name = "TMP_CAS_" + std::to_string(counter) + "_";
		if (names.count(var_name) == 0) {
			function.body->variables.push_back(std::make_unique<VariableDeclaration>(var_name, Type::bool_type(), false));
			return *function.body->variables.back();
		} else {
			++counter;
		}
	}
	throw std::logic_error("This is bad.");
}

struct CasRemovalVisitor : public NonConstVisitor {
	std::unique_ptr<Statement>* current_owner = nullptr;
	Function* current_function = nullptr;

	void visit(VariableDeclaration& /*node*/) { throw std::logic_error("Unexpected invocation: visit(VariableDeclaration&)"); }
	void visit(Expression& /*node*/) { throw std::logic_error("Unexpected invocation: visit(Expression&)"); }
	void visit(BooleanValue& /*node*/) { throw std::logic_error("Unexpected invocation: visit(BooleanValue&)"); }
	void visit(NullValue& /*node*/) { throw std::logic_error("Unexpected invocation: visit(NullValue&)"); }
	void visit(EmptyValue& /*node*/) { throw std::logic_error("Unexpected invocation: visit(EmptyValue&)"); }
	void visit(NDetValue& /*node*/) { throw std::logic_error("Unexpected invocation: visit(NDetValue&)"); }
	void visit(VariableExpression& /*node*/) { throw std::logic_error("Unexpected invocation: visit(VariableExpression&)"); }
	void visit(NegatedExpression& /*node*/) { throw std::logic_error("Unexpected invocation: visit(NegatedExpression&)"); }
	void visit(BinaryExpression& /*node*/) { throw std::logic_error("Unexpected invocation: visit(BinaryExpression&)"); }
	void visit(Dereference& /*node*/) { throw std::logic_error("Unexpected invocation: visit(Dereference&)"); }
	void visit(InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: visit(InvariantExpression&)"); }
	void visit(InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: visit(InvariantActive&)"); }
	
	void handle_statement(std::unique_ptr<Statement>& stmt) {
		this->current_owner = &stmt;
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
		for (auto& branch : node.branches) {
			branch->accept(*this);
		}
	}
	void visit(IfThenElse& node) {
		std::unique_ptr<Statement>* my_owner = this->current_owner;
		node.ifBranch->accept(*this);
		node.elseBranch->accept(*this);
		this->current_owner = my_owner;

		// handle CAS as condition
		CasFinderVisitor visitor;
		node.expr->accept(visitor);
		if (visitor.found != nullptr) {
			assert(!visitor.found_low_level);
			auto& cas = *visitor.found;
			assert(current_owner);
			assert(current_owner->get() == &node);
			assert(this->current_function);
			VariableDeclaration& tmp = make_tmp_var(*this->current_function);
			if (cas.elems.size() == 1) {
				auto& dst = *cas.elems.at(0).dst;
				auto& cmp = *cas.elems.at(0).cmp;
				auto& src = *cas.elems.at(0).src;
				// replace cas with: if (dst == cmp) { dst = src; } else { skip; }
				auto condition = std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, cola::copy(dst), cola::copy(cmp));
				auto assignment = std::make_unique<Assignment>(cola::copy(dst), cola::copy(src));
				auto flag_true = std::make_unique<Assignment>(std::make_unique<VariableExpression>(tmp), std::make_unique<BooleanValue>(true));
				auto flag_false = std::make_unique<Assignment>(std::make_unique<VariableExpression>(tmp), std::make_unique<BooleanValue>(false));
				auto branch_true = std::make_unique<Scope>(std::make_unique<Sequence>(std::move(assignment), std::move(flag_true)));
				auto branch_false = std::make_unique<Scope>(std::move(flag_false));
				auto ite = std::make_unique<Atomic>(std::make_unique<Scope>(std::make_unique<IfThenElse>(std::move(condition), std::move(branch_true), std::move(branch_false))));
				node.expr = std::make_unique<VariableExpression>(tmp);
				auto replacement = std::make_unique<Sequence>(std::move(ite), std::move(*current_owner));
				*current_owner = std::move(replacement);

			} else {
				throw std::logic_error("not yet implemented: desugaring of kCAS with k!=1");
			}
		}

	}
	void visit(Loop& node) {
		node.body->accept(*this);
	}
	void visit(While& node) {
		node.body->accept(*this);
	}
	
	void visit(Skip& /*node*/) { /* do nothing */ }
	void visit(Break& /*node*/) { /* do nothing */ }
	void visit(Continue& /*node*/) { /* do nothing */ }
	void visit(Assume& node) {
		assert(!contains_cas(*node.expr));
	}
	void visit(Assert& /*node*/) { /* do nothing */ }
	void visit(Return& /*node*/) { /* do nothing */ }
	void visit(Malloc& /*node*/) { /* do nothing */ }
	void visit(Assignment& /*node*/) { /* do nothing */ }
	void visit(Enter& /*node*/) { /* do nothing */ }
	void visit(Exit& /*node*/) { /* do nothing */ }
	void visit(Macro& /*node*/) { /* do nothing */ }
	
	void visit(CompareAndSwap& cas) {
		assert(current_owner);
		assert(current_owner->get() == &cas);
		if (cas.elems.size() == 1) {
			auto& dst = *cas.elems.at(0).dst;
			auto& cmp = *cas.elems.at(0).cmp;
			auto& src = *cas.elems.at(0).src;
			// replace cas with: if (dst == cmp) { dst = src; } else { skip; }
			auto condition = std::make_unique<BinaryExpression>(BinaryExpression::Operator::EQ, cola::copy(dst), cola::copy(cmp));
			auto assignment = std::make_unique<Assignment>(cola::copy(dst), cola::copy(src));
			auto replacement = std::make_unique<Atomic>(std::make_unique<Scope>(std::make_unique<IfThenElse>(std::move(condition), std::make_unique<Scope>(std::move(assignment)), std::make_unique<Scope>(std::make_unique<Skip>()))));
			*current_owner = std::move(replacement);

		} else {
			throw std::logic_error("not yet implemented: desugaring of kCAS with k!=1");
		}
	}
	
	void visit(Function& function) {
		if (function.body) {
			this->current_function = &function;
			function.body->accept(*this);
		}
	}
	void visit(Program& program) {
		program.initializer->accept(*this);
		for (auto& function : program.functions) {
			function->accept(*this);
		}
	}
};

void cola::remove_cas(Program& program) {
	CasRemovalVisitor visitor;
	program.accept(visitor);
}
