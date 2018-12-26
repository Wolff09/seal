#include "simplification/simplify.hpp"

#include "ast/ast.hpp"
#include "util/util.hpp"


struct RemoveConditionalsVisitor final : public BaseVisitor {
	std::unique_ptr<Statement> replacement;
	bool replacementNeeded = false;

	void visit(Program& program) {
		program.initalizer->accept(*this);
		for (auto& func : program.functions) {
			func->accept(*this);
		}
	}

	void visit(Function& function) {
		function.body->accept(*this);
	}

	void visit(Atomic& atomic) {
		atomic.body->accept(*this);
	}

	void visit(Choice& choice) {
		for (auto& scope : choice.branches) {
			scope->accept(*this);
		}
	}
	void visit(Loop& loop) {
		loop.body->accept(*this);
	}

	void prependAssumption(Scope& scope, std::unique_ptr<Expression> expr) {
		auto assume = std::make_unique<Assume>(std::move(expr));
		auto seq = std::make_unique<Sequence>(std::move(assume), std::move(scope.body));
		scope.body = std::move(seq);
	}

	void visit(IfThenElse& ite) {
		if (ite.elseBranch) {
			ite.elseBranch->accept(*this);
			auto expr = std::make_unique<NegatedExpression>(cola::copy(*ite.expr));
			prependAssumption(*ite.elseBranch, std::move(expr));
		}

		ite.ifBranch->accept(*this);
		prependAssumption(*ite.ifBranch, std::move(ite.expr));

		auto choice = std::make_unique<Choice>();
		choice->branches.reserve(2);
		choice->branches.push_back(std::move(ite.ifBranch));
		choice->branches.push_back(std::move(ite.elseBranch));
		
		replacement = std::move(choice);
		replacementNeeded = true;
	}

	void visit(While& whl) {
		whl.body->accept(*this);

		auto expr = std::make_unique<NegatedExpression>(cola::copy(*whl.expr));
		prependAssumption(*whl.body, std::move(whl.expr));
		auto loop = std::make_unique<Loop>(std::move(whl.body));
		auto assume = std::make_unique<Assume>(std::move(expr));
		auto seq = std::make_unique<Sequence>(std::move(loop), std::move(assume));
		
		replacement = std::move(seq);
		replacementNeeded = true;
	}

	void acceptAndReplaceIfNeeded(std::unique_ptr<Statement>& uptr) {
		uptr->accept(*this);
		if (replacementNeeded) {
			assert(replacement);
			uptr = std::move(replacement);
			replacement.reset();
			replacementNeeded = false;
		} 
	}

	void visit(Scope& scope) {
		acceptAndReplaceIfNeeded(scope.body);
	}

	void visit(Sequence& sequence) {
		acceptAndReplaceIfNeeded(sequence.first);
		acceptAndReplaceIfNeeded(sequence.second);
	}
};


void cola::remove_conditionals(Program& program) {
	RemoveConditionalsVisitor().visit(program);
}
