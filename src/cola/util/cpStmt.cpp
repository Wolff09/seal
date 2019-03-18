#include "cola/util.hpp"

using namespace cola;


struct CopyCommandVisitor final : public Visitor {
	std::unique_ptr<Command> result;

	void visit(const VariableDeclaration& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const VariableDeclaration&)"); }
	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const EmptyValue&)"); }
	void visit(const MaxValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const MaxValue&)"); }
	void visit(const MinValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const MinValue&)"); }
	void visit(const NDetValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const NDetValue&)"); }
	void visit(const VariableExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const VariableExpression&)"); }
	void visit(const NegatedExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const BinaryExpression&)"); }
	void visit(const Dereference& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const Dereference&)"); }
	void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const InvariantActive&)"); }
	void visit(const Sequence& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const Sequence&)"); }
	void visit(const Scope& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const Scope&)"); }
	void visit(const Atomic& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const Atomic&)"); }
	void visit(const Choice& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const Choice&)"); }
	void visit(const IfThenElse& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const IfThenElse&)"); }
	void visit(const Loop& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const Loop&)"); }
	void visit(const While& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const While&)"); }
	void visit(const Function& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) override { throw std::logic_error("Unexpected invocation: CopyCommandVisitor::visit(const Program&)"); }

	void visit(const Skip& /*node*/) override {
		this->result = std::make_unique<Skip>();
	}
	void visit(const Break& /*node*/) override {
		this->result = std::make_unique<Break>();
	}
	void visit(const Continue& /*node*/) override {
		this->result = std::make_unique<Continue>();
	}
	void visit(const Return& node) override {
		if (node.expr) {
			this->result = std::make_unique<Return>(cola::copy(*node.expr));
		} else {
			this->result = std::make_unique<Return>();
		}
	}
	void visit(const Assume& node) override {
		this->result = std::make_unique<Assume>(cola::copy(*node.expr));
	}
	void visit(const Assert& node) override {
		this->result = std::make_unique<Assert>(cola::copy(*node.inv));
	}
	void visit(const Malloc& node) override {
		this->result = std::make_unique<Malloc>(node.lhs);
	}
	void visit(const Assignment& node) override {
		this->result = std::make_unique<Assignment>(cola::copy(*node.lhs), cola::copy(*node.rhs));
	}
	void visit(const Enter& node) override {
		auto result = std::make_unique<Enter>(node.decl);
		for (const auto& arg : node.args) {
			result->args.push_back(cola::copy(*arg));
		}
		this->result = std::move(result);
	}
	void visit(const Exit& node) override {
		this->result = std::make_unique<Exit>(node.decl);
	}
	void visit(const Macro& node) override {
		auto result = std::make_unique<Macro>(node.decl);
		for (const auto& arg : node.args) {
			result->args.push_back(cola::copy(*arg));
		}
		this->result = std::move(result);
	}
	void visit(const CompareAndSwap& node) override {
		auto result = std::make_unique<CompareAndSwap>();
		for (const auto& [dst, cmp, src] : node.elems) {
			result->elems.push_back({ cola::copy(*dst), cola::copy(*cmp), cola::copy(*src),  });
		}
		this->result = std::move(result);
	}
};

std::unique_ptr<Command> cola::copy(const Command& cmd) {
	CopyCommandVisitor visitor;
	cmd.accept(visitor);
	return std::move(visitor.result);
}
