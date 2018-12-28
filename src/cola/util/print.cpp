#include "cola/util.hpp"

#include <iostream> // TODO: remove?
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

	PrintVisitor(std::ostream& stream) : stream(stream), indent(stream) {}

	std::string typenameof(const Type& type) {
		std::string name = type.name;
		if (type.sort == Sort::PTR) {
			name.pop_back(); // remove trainling * for pointer types
		}
		return name;
	}

	void print_type(const Type& type) {
		stream << indent++ << "struct " << typenameof(type) << " {" << std::endl;
		for (const auto& pair : type.fields) {
			stream << indent << typenameof(pair.second.get())  << " " << pair.first << ";" << std::endl;
		}
		stream << --indent << "};" << std::endl << std::endl;
	}


	void visit(const Program& program) {
		stream << "/* program name: " << program.name << " */";
		stream << std::endl;

		for (const auto& type : program.types) {
			print_type(*type);
		}
		stream << std::endl;

		for (const auto& decl : program.variables) {
			decl->accept(*this);
		}
		stream << std::endl;

		program.initalizer->accept(*this);
		for (const auto& function :  program.functions) {
			function->accept(*this);
		}
	}

	void visit(const Function& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Function&)"); }

	void visit(const VariableDeclaration& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const VariableDeclaration&)"); }


	void visit(const Sequence& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Sequence&)"); }
	void visit(const Scope& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Scope&)"); }
	void visit(const Atomic& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Atomic&)"); }
	void visit(const Choice& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Choice&)"); }
	void visit(const IfThenElse& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const IfThenElse&)"); }
	void visit(const Loop& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Loop&)"); }
	void visit(const While& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const While&)"); }
	
	void visit(const Skip& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Skip&)"); }
	void visit(const Break& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Break&)"); }
	void visit(const Continue& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Continue&)"); }
	void visit(const Assume& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Assume&)"); }
	void visit(const Assert& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Assert&)"); }
	void visit(const Return& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Return&)"); }
	void visit(const Malloc& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Malloc&)"); }
	void visit(const Assignment& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Assignment&)"); }
	void visit(const Enter& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Enter&)"); }
	void visit(const Exit& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Exit&)"); }
	void visit(const Macro& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Macro&)"); }
	void visit(const CompareAndSwap& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const CompareAndSwap&)"); }

	void visit(const Expression& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Expression&)"); }
	void visit(const BooleanValue& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const BooleanValue&)"); }
	void visit(const NullValue& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const NullValue&)"); }
	void visit(const EmptyValue& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const EmptyValue&)"); }
	void visit(const NDetValue& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const NDetValue&)"); }
	void visit(const VariableExpression& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const VariableExpression&)"); }
	void visit(const NegatedExpression& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const BinaryExpression&)"); }
	void visit(const Dereference& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const Dereference&)"); }
	void visit(const InvariantExpression& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& node) { throw std::logic_error("not yet implemented: PrintVisitor:::visit(const InvariantActive&)"); }
};

void cola::print(const Program& program, std::ostream& stream) {
	PrintVisitor(stream).visit(program);
}
