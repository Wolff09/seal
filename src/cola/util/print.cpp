#include "cola/util.hpp"

#include <sstream>
#include "cola/ast.hpp"

using namespace cola;


struct PrintExpressionVisitor final : public Visitor {
	std::ostream& stream;
	std::size_t precedence = 0;

	PrintExpressionVisitor(std::ostream& stream) : stream(stream) {}

	void visit(const CompareAndSwap& com) {
		std::stringstream dst, cmp, src;
		PrintExpressionVisitor dstVisitor(dst), cmpVisitor(cmp), srcVisitor(src);
		assert(!com.elems.empty());
		com.elems.at(0).dst->accept(dstVisitor);
		com.elems.at(0).cmp->accept(cmpVisitor);
		com.elems.at(0).src->accept(srcVisitor);
		for (std::size_t i = 1; i < com.elems.size(); i++) {
			dst << ", ";
			cmp << ", ";
			src << ", ";
			com.elems.at(i).dst->accept(dstVisitor);
			com.elems.at(i).cmp->accept(cmpVisitor);
			com.elems.at(i).src->accept(srcVisitor);
		}
		std::string dstResult = dst.str();
		std::string cmpResult = cmp.str();
		std::string srcResult = src.str();
		if (com.elems.size() > 1) {
			dstResult = "<" + dstResult + ">";
			cmpResult = "<" + cmpResult + ">";
			srcResult = "<" + srcResult + ">";
		}
		stream << "CAS(" << dstResult << ", " << cmpResult << ", " << srcResult << ")";
	}


	void set_precedence_op(BinaryExpression::Operator op) {
		// TODO: correct
		switch (op) {
			case BinaryExpression::Operator::EQ: precedence = 1; break;
			case BinaryExpression::Operator::NEQ: precedence = 1; break;
			case BinaryExpression::Operator::LEQ: precedence = 1; break;
			case BinaryExpression::Operator::LT: precedence = 1; break;
			case BinaryExpression::Operator::GEQ: precedence = 1; break;
			case BinaryExpression::Operator::GT: precedence = 1; break;
			case BinaryExpression::Operator::AND: precedence = 0; break;
			case BinaryExpression::Operator::OR: precedence = 0; break;
		}
	}

	void set_precedence_neg() {
		precedence = 2;
	}

	void set_precedence_deref() {
		precedence = 3;
	}

	void set_precedence_immi() {
		precedence = 4;
	}


	void visit(const Expression& /*expr*/) {
		throw std::logic_error("Unexpected invocation (PrintVisitor:::visit(const Expression&))");
	}

	void visit(const BooleanValue& expr) {
		stream << (expr.value ? "true" : "false");
		set_precedence_immi();
	}

	void visit(const NullValue& /*expr*/) {
		stream << "NULL";
		set_precedence_immi();
	}

	void visit(const EmptyValue& /*expr*/) {
		stream << "EMPTY";
		set_precedence_immi();
	}

	void visit(const NDetValue& /*expr*/) {
		stream << "*";
		set_precedence_immi();
	}

	void visit(const VariableExpression& expr) {
		stream << expr.decl.name;
		set_precedence_immi();
	}

	void print_sub_expression(const Expression& expr) {
		std::stringstream my_stream;
		PrintExpressionVisitor subprinter(my_stream);
		expr.accept(subprinter);
		std::string expr_string = my_stream.str();

		if (subprinter.precedence <= precedence) {
			stream << "(" << expr_string << ")";
		} else {
			stream << expr_string;
		}
	}

	void visit(const NegatedExpression& expr) {
		// TODO: check when parenthesis are needed
		set_precedence_neg();
		stream << "!";
		print_sub_expression(*expr.expr);
	}

	void visit(const BinaryExpression& expr) {
		// TODO: check when parenthesis are needed
		set_precedence_op(expr.op);
		print_sub_expression(*expr.lhs);
		stream << " " << toString(expr.op) << " ";
		print_sub_expression(*expr.rhs);
	}

	void visit(const Dereference& expr) {
		// TODO: check when parenthesis are needed
		set_precedence_deref();
		print_sub_expression(*expr.expr);
		stream << "->" << expr.fieldname;
	}

	void visit(const InvariantExpression& expr) {
		expr.expr->accept(*this);
	}

	void visit(const InvariantActive& expr) {
		stream << "active(";
		expr.expr->accept(*this);
		stream << ")";
	}

	void visit(const Program& /*stmt*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Program&))"); }
	void visit(const Function& /*stmt*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Function&))"); }
	void visit(const VariableDeclaration& /*stmt*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const VariableDeclaration&))"); }
	void visit(const Scope& /*stmt*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Scope&))"); }
	void visit(const Sequence& /*stmt*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Sequence&))"); }
	void visit(const Atomic& /*stmt*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Atomic&))"); }
	void visit(const Choice& /*stmt*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Choice&))"); }
	void visit(const IfThenElse& /*stmt*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const IfThenElse&))"); }
	void visit(const Loop& /*stmt*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Loop&))"); }
	void visit(const While& /*stmt*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const While&))"); }
	void visit(const Skip& /*com*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Skip&))"); }
	void visit(const Break& /*com*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Break&))"); }
	void visit(const Continue& /*com*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Continue&))"); }
	void visit(const Assume& /*com*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Assume&))"); }
	void visit(const Assert& /*com*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Assert&))"); }
	void visit(const Return& /*com*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Return&))"); }
	void visit(const Malloc& /*com*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Malloc&))"); }
	void visit(const Assignment& /*com*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Assignment&))"); }
	void visit(const Enter& /*com*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Enter&))"); }
	void visit(const Exit& /*com*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Exit&))"); }
	void visit(const Macro& /*com*/) { throw std::logic_error("Unexpected invocation (PrintExpressionVisitor::visit(const Macro&))"); }
};

struct PrintVisitor final : public Visitor {
	std::ostream& stream;
	Indent indent;
	bool scope_sameline = false;
	PrintExpressionVisitor exprinter;

	PrintVisitor(std::ostream& stream) : stream(stream), indent(stream), exprinter(stream) {}

	std::string typenameof(const Type& type) {
		std::string name = type.name;
		if (type.sort == Sort::PTR) {
			name.pop_back(); // remove trainling * for pointer types
		}
		return name;
	}

	void print_class(const Type& type) {
		stream << std::endl << indent++ << "struct " << typenameof(type) << " {" << std::endl;
		for (const auto& pair : type.fields) {
			stream << indent << typenameof(pair.second.get())  << " " << pair.first << ";" << std::endl;
		}
		stream << --indent << "};" << std::endl;
	}


	void visit(const Program& program) {
		stream << "// BEGIN program " << program.name << std::endl;

		for (const auto& type : program.types) {
			print_class(*type);
		}
		stream << std::endl;

		for (const auto& decl : program.variables) {
			decl->accept(*this);
			stream << ";" << std::endl;
		}

		program.initializer->accept(*this);
		for (const auto& function :  program.functions) {
			function->accept(*this);
		}

		stream << std::endl << "// END program " << program.name << std::endl;
	}

	void print_scope(const Scope& scope) {
		scope_sameline = true;
		scope.accept(*this);
	}

	void visit(const Function& function) {
		std::string modifier;
		bool has_body;

		switch (function.kind) {
			case Function::INTERFACE: modifier = ""; has_body = true; assert(function.body); break;
			case Function::SMR: modifier = "extern "; has_body = false; assert(!function.body); break;
			case Function::MACRO: modifier = "inline "; has_body = true; assert(function.body); break;
		}

		stream << std::endl << modifier << function.return_type.name << " " << function.name << "(";
		if (function.args.size() > 0) {
			function.args.at(0)->accept(*this);
			for (std::size_t i = 1; i < function.args.size(); i++) {
				stream << ", ";
				function.args.at(i)->accept(*this);
			}
		}
		stream << ")";
		if (has_body) {
			stream << " ";
			print_scope(*function.body);
		} else {
			stream << ";";
		}
		stream << std::endl;
	}

	void visit(const VariableDeclaration& decl) {
		// output without indent and delimiter
		stream << decl.type.name << " " << decl.name;
	}


	void visit(const Scope& scope) {
		bool sameline = scope_sameline;
		scope_sameline = false;
		if (!sameline) {
			stream << indent;
		}
		stream << "{" << std::endl;
		indent++;
		for (const auto& var : scope.variables) {
			stream << indent;
			var->accept(*this);
			stream << ";" << std::endl;
		}
		scope.body->accept(*this);
		stream << --indent << "}";
		if (!sameline) {
			stream << std::endl;
		}

	}

	void visit(const Sequence& sequence) {
		sequence.first->accept(*this);
		sequence.second->accept(*this);
	}

	void print_annotation(const AnnotatedStatement& stmt) {
		if (stmt.annotation) {
			stream << indent << "@invariant(";
			stmt.annotation->accept(exprinter);
			stream << ")" << std::endl;
		}
	}

	void visit(const Atomic& atomic) {
		print_annotation(atomic);
		stream << indent << "atomic ";
		print_scope(*atomic.body);
		stream << std::endl;
	}
	
	void visit(const Choice& choice) {
		stream << indent << "choose ";
		for (const auto& scope : choice.branches) {
			print_scope(*scope);
		}
		stream << std::endl;
	}
	
	void visit(const IfThenElse& ite) {
		print_annotation(ite);
		stream << indent << "if (";
		ite.expr->accept(exprinter);
		stream << ") ";
		print_scope(*ite.ifBranch);
		if (ite.elseBranch) {
			stream << " else ";
			print_scope(*ite.elseBranch);
		}
		stream << std::endl;
	}

	void visit(const Loop& loop) {
		stream << indent << "loop ";
		print_scope(*loop.body);
		stream << std::endl;
	}

	void visit(const While& whl) {
		print_annotation(whl);
		stream << indent << "while (";
		whl.expr->accept(exprinter);
		stream << ") ";
		print_scope(*whl.body);
		stream << std::endl;
	}

	void visit(const Skip& com) {
	print_annotation(com);
		stream << indent << "skip;" << std::endl;
	}

	void visit(const Break& com) {
	print_annotation(com);
		stream << indent << "break;" << std::endl;
	}

	void visit(const Continue& com) {
	print_annotation(com);
		stream << indent << "continue;" << std::endl;
	}

	void visit(const Assume& com) {
		print_annotation(com);
		stream << indent << "assume(";
		com.expr->accept(exprinter);
		stream << ");";
		stream << " // " << com.id;
		stream << std::endl;
	}

	void visit(const Assert& com) {
		print_annotation(com);
		stream << indent << "assert(";
		com.inv->accept(exprinter);
		stream << ");" << std::endl;
	}

	void visit(const Return& com) {
		print_annotation(com);
		stream << indent << "return";
		if (com.expr) {
			stream << " ";
			com.expr->accept(exprinter);
		}
		stream << ";" << std::endl;
	}

	void visit(const Malloc& com) {
		print_annotation(com);
		stream << indent << com.lhs.name << " = malloc;" << std::endl;
	}

	void visit(const Assignment& com) {
		print_annotation(com);
		stream << indent;
		com.lhs->accept(exprinter);
		stream << " = ";
		com.rhs->accept(exprinter);
		stream << ";" << std::endl;
	}

	void visit(const Enter& com) {
		print_annotation(com);
		stream << indent << "enter " << com.decl.name << "(";
		if (!com.args.empty()) {
			com.args.at(0)->accept(exprinter);
			for (std::size_t i = 1; i < com.args.size(); i++) {
				stream << ", ";
				com.args.at(i)->accept(exprinter);
			}
		}
		stream << ");" << std::endl;
	}

	void visit(const Exit& com) {
		print_annotation(com);
		stream << indent << "exit " << com.decl.name << ";" << std::endl;
	}

	void visit(const Macro& com) {
		print_annotation(com);
		stream << indent << com.decl.name << "(";
		if (!com.args.empty()) {
			com.args.at(0)->accept(exprinter);
			for (std::size_t i = 1; i < com.args.size(); i++) {
				stream << ", ";
				com.args.at(i)->accept(exprinter);
			}
		}
		stream << ");" << std::endl;
	}

	void visit(const CompareAndSwap& com) {
		print_annotation(com);
		stream << indent;
		com.accept(exprinter);
		stream << ";" << std::endl;
	}


	void visit(const Expression& /*expr*/) { throw std::logic_error("Unexpected invocation (PrintVisitor:::visit(const Expression&))"); }
	void visit(const BooleanValue& /*expr*/) { throw std::logic_error("Unexpected invocation (PrintVisitor::visit(const BooleanValue&))"); }
	void visit(const NullValue& /*expr*/) { throw std::logic_error("Unexpected invocation (PrintVisitor::visit(const NullValue&))"); }
	void visit(const EmptyValue& /*expr*/) { throw std::logic_error("Unexpected invocation (PrintVisitor::visit(const EmptyValue&))"); }
	void visit(const NDetValue& /*expr*/) { throw std::logic_error("Unexpected invocation (PrintVisitor::visit(const NDetValue&))"); }
	void visit(const VariableExpression& /*expr*/) { throw std::logic_error("Unexpected invocation (PrintVisitor::visit(const VariableExpression&))"); }
	void visit(const NegatedExpression& /*expr*/) { throw std::logic_error("Unexpected invocation (PrintVisitor::visit(const NegatedExpression&))"); }
	void visit(const BinaryExpression& /*expr*/) { throw std::logic_error("Unexpected invocation (PrintVisitor::visit(const BinaryExpression&))"); }
	void visit(const Dereference& /*expr*/) { throw std::logic_error("Unexpected invocation (PrintVisitor::visit(const Dereference&))"); }
	void visit(const InvariantExpression& /*expr*/) { throw std::logic_error("Unexpected invocation (PrintVisitor::visit(const InvariantExpression&))"); }
	void visit(const InvariantActive& /*expr*/) { throw std::logic_error("Unexpected invocation (PrintVisitor::visit(const InvariantActive&))"); }

};

void cola::print(const Program& program, std::ostream& stream) {
	PrintVisitor(stream).visit(program);
}

void cola::print(const Expression& expression, std::ostream& stream) {
	PrintExpressionVisitor visitor(stream);
	expression.accept(visitor);
}

void cola::print(const Command& command, std::ostream& stream) {
	PrintVisitor visitor(stream);
	command.accept(visitor);
}
