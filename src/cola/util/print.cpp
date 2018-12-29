#include "cola/util.hpp"

#include <sstream>
#include "cola/ast.hpp"

using namespace cola;


struct Indent {
	static const std::size_t indention_size = 4;
	std::size_t current_indent = 0;
	std::ostream& stream;
	Indent(std::ostream& stream) : stream(stream) {}
	Indent& operator++() { current_indent++; return *this; }
	Indent& operator--() { current_indent--; return *this; }
	Indent operator++(int) { Indent result(*this); ++(*this); return result; }
	Indent operator--(int) { Indent result(*this); --(*this); return result; }
	void print(std::ostream& stream) const { stream << std::string(current_indent*indention_size, ' '); }
	void operator()() const { print(stream); }
};

std::ostream& operator<<(std::ostream& stream, const Indent indent) { indent.print(stream); return stream; }

struct PrintVisitor final : public Visitor {
	std::ostream& stream;
	Indent indent;
	bool scope_sameline = false;

	PrintVisitor(std::ostream& stream) : stream(stream), indent(stream) {}

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

		program.initalizer->accept(*this);
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
		switch (function.kind) {
			case Function::INTERFACE: modifier = ""; break;
			case Function::SMR: modifier = "extern "; break;
			case Function::MACRO: modifier = "inline "; break;
		}

		stream << std::endl << modifier << function.return_type.name << " " << function.name << "(";
		if (function.args.size() > 0) {
			function.args.at(0)->accept(*this);
			for (std::size_t i = 1; i < function.args.size(); i++) {
				stream << ", ";
				function.args.at(i)->accept(*this);
			}
		}
		stream << ") ";
		print_scope(*function.body);
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
			stmt.annotation->accept(*this);
			stream << ")";
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
		ite.expr->accept(*this);
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
		whl.expr->accept(*this);
		stream << ") " << std::endl;
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
		com.expr->accept(*this);
		stream << ");" << std::endl;
	}

	void visit(const Assert& com) {
		print_annotation(com);
		stream << indent << "assert(";
		com.inv->accept(*this);
		stream << ");" << std::endl;
	}

	void visit(const Return& com) {
		print_annotation(com);
		stream << indent << "return";
		if (com.expr) {
			stream << " ";
			com.expr->accept(*this);
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
		com.lhs->accept(*this);
		stream << " = ";
		com.rhs->accept(*this);
		stream << ";" << std::endl;
	}

	void visit(const Enter& com) {
		print_annotation(com);
		stream << indent << "enter " << com.decl.name << "(";
		if (!com.args.empty()) {
			com.args.at(0)->accept(*this);
			for (std::size_t i = 1; i < com.args.size(); i++) {
				stream << ", ";
				com.args.at(i)->accept(*this);
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
			com.args.at(0)->accept(*this);
			for (std::size_t i = 1; i < com.args.size(); i++) {
				stream << ", ";
				com.args.at(i)->accept(*this);
			}
		}
		stream << ");" << std::endl;
	}

	void visit(const CompareAndSwap& com) {
		// TODO: CAS as statement will not print correctly
		print_annotation(com);
		std::stringstream dst, cmp, src;
		PrintVisitor dstVisitor(dst), cmpVisitor(cmp), srcVisitor(src);
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


	void visit(const Expression& /*expr*/) {
		throw std::logic_error("Unexpected invocation (PrintVisitor:::visit(const Expression&))");
	}

	void visit(const BooleanValue& expr) {
		stream << (expr.value ? "true" : "false");
	}

	void visit(const NullValue& /*expr*/) {
		stream << "NULL";
	}

	void visit(const EmptyValue& /*expr*/) {
		stream << "EMPTY";
	}

	void visit(const NDetValue& /*expr*/) {
		stream << "*";
	}

	void visit(const VariableExpression& expr) {
		stream << expr.decl.name;
	}

	void visit(const NegatedExpression& expr) {
		// TODO: check when parenthesis are needed
		stream << "!(";
		expr.expr->accept(*this);
		stream << ")";
	}

	void visit(const BinaryExpression& expr) {
		// TODO: check when parenthesis are needed
		stream << "(";
		expr.lhs->accept(*this);
		stream << ") " << toString(expr.op) << " (";
		expr.rhs->accept(*this);
		stream << ")";
	}

	void visit(const Dereference& expr) {
		// TODO: check when parenthesis are needed
		stream << "(";
		stream << ")->" << expr.fieldname;
		throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Dereference&)");
	}

	void visit(const InvariantExpression& expr) {
		expr.expr->accept(*this);
	}

	void visit(const InvariantActive& expr) {
		stream << "active(";
		expr.expr->accept(*this);
		stream << ")";
	}

};

void cola::print(const Program& program, std::ostream& stream) {
	PrintVisitor(stream).visit(program);
}
