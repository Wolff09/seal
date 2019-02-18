#include "types/cave.hpp"
#include "types/guarantees.hpp"
#include "cola/ast.hpp"
#include "cola/util.hpp"
#include "types/error.hpp"
#include <iostream>
#include <sstream>

using namespace cola;
using namespace prtypes;


void print_delimiter(std::ostream& stream, std::string text) {
	stream << "// ----------------------------------------------------------------------" << std::endl;
	stream << "// " << text << std::endl;
	stream << "// ----------------------------------------------------------------------" << std::endl;
}

// TODO: forbid annotated invariants at commands/statements
// TODO: instrumentation still missing (base observer)

struct CaveOutputVisitor : public Visitor {
	std::ostream& stream;
	Indent indent;

	CaveOutputVisitor(std::ostream& stream) : stream(stream), indent(stream) {}

	std::string type2cave(const Type& type) {
		if (type.sort == Sort::DATA) {
			return "int";
		} else {
			return type.name;
		}
	}

	void print_type_def(const Type& type) {
		stream << "class " << type.name << " {" << std::endl;
		indent++;
		for (const auto& [fiedl_name, type] : type.fields) {
			stream << indent << type2cave(type) << " " << fiedl_name << ";" << std::endl;
		}
		indent--;
		stream << "}" << std::endl << std::endl;
	}

	void print_var_def(const VariableDeclaration& decl, bool with_semicolon=true) {
		stream << indent << type2cave(decl.type) << " " << decl.name << (with_semicolon ? ";" : "") << std::endl;
	}

	void print_initializer(const Function& init) {
		// TODO: does this work? Or do we need to wrap the shared variables into an object?
		stream << "resource r() {" << std::endl;
		indent++;
		stream << indent << "constructor ";
		assert(init.body);
		init.body->accept(*this);
		indent--;
		stream << indent << "}" << std::endl;
	}	

	void visit(const VariableDeclaration& /*node*/) { throw std::logic_error("not yet implemented: visit(const VariableDeclaration&)"); }
	void visit(const Expression& /*node*/) { throw std::logic_error("not yet implemented: visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) { throw std::logic_error("not yet implemented: visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) { throw std::logic_error("not yet implemented: visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) { throw std::logic_error("not yet implemented: visit(const EmptyValue&)"); }
	void visit(const NDetValue& /*node*/) { throw std::logic_error("not yet implemented: visit(const NDetValue&)"); }
	void visit(const VariableExpression& /*node*/) { throw std::logic_error("not yet implemented: visit(const VariableExpression&)"); }
	void visit(const NegatedExpression& /*node*/) { throw std::logic_error("not yet implemented: visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& /*node*/) { throw std::logic_error("not yet implemented: visit(const BinaryExpression&)"); }
	void visit(const Dereference& /*node*/) { throw std::logic_error("not yet implemented: visit(const Dereference&)"); }
	
	void visit(const Scope& scope) {
		stream << "{" << std::endl;
		indent++;
		for (const auto& decl : scope.variables) {
			print_var_def(*decl);
		}
		assert(scope.body);
		scope.body->accept(*this);
		indent--;
		stream << indent << "}" << std::endl;
	}

	void visit(const Atomic& atomic) {
		stream << indent << "atomic ";
		assert(atomic.body);
		atomic.body->accept(*this);
	}

	void visit(const Sequence& seq) {
		assert(seq.first);
		assert(seq.second);
		seq.first->accept(*this);
		seq.second->accept(*this);
	}

	void visit(const Choice& choice) {
		stream << indent << "if (*) ";
		assert(choice.branches.size() > 0);
		for (const auto& branch : choice.branches) {
			assert(branch);
			branch->accept(*this);
		}
	}

	void visit(const Loop& loop) {
		stream << indent << "while (*) ";
		assert(loop.body);
		loop.body->accept(*this);
	}

	void visit(const IfThenElse& /*node*/) {
		raise_error<UnsupportedConstructError>("'if' not supported in the translation to CAVE");
	}
	void visit(const While& /*node*/) {
		raise_error<UnsupportedConstructError>("'while' not supported in the translation to CAVE");
	}


	void visit(const Break& /*node*/) {
		raise_error<UnsupportedConstructError>("'break' not supported in the translation to CAVE");
	}
	
	void visit(const Continue& /*node*/) {
		raise_error<UnsupportedConstructError>("'continue' not supported in the translation to CAVE");
	}
	
	void visit(const Return& /*node*/) {
		// TODO: implement, needed for linearizability checks
		raise_error<UnsupportedConstructError>("'return' not supported in the translation to CAVE");
	}
	
	void visit(const Macro& /*node*/) { 
		raise_error<UnsupportedConstructError>("calls to macros are not supported in the translation to CAVE");
	}

	void visit(const CompareAndSwap& /*node*/) {
		raise_error<UnsupportedConstructError>("'CAS' to macros are not supported in the translation to CAVE");
	}

	void visit(const Skip& /*node*/) {
		/* do nothing */
	}

	void visit(const Assume& assume) {
		stream << indent << "assume(";
		assert(assume.expr);
		cola::print(*assume.expr, stream);
		stream << ");" << std::endl;
	}

	void visit(const InvariantExpression& invariant) {
		stream << indent << "if (!(";
		assert(invariant.expr);
		cola::print(*invariant.expr, stream);
		stream << ")) { fail(); }" << std::endl;
	}
	void visit(const InvariantActive& invariant) {
		stream << indent << "assertActive(";
		assert(invariant.expr);
		cola::print(*invariant.expr, stream);
		stream << ");" << std::endl;
	}

	void visit(const Assert& assrt) {
		assert(assrt.inv);
		assrt.inv->accept(*this);
	}

	void visit(const Malloc& malloc) {
		stream << indent << malloc.lhs.name << " = new();" << std::endl;
	}

	void visit(const Assignment& assign) {
		stream << indent;
		assert(assign.lhs);
		cola::print(*assign.lhs, stream);
		stream << " = ";
		assert(assign.rhs);
		cola::print(*assign.rhs, stream);
		stream << ";" << std::endl;
	}

	void visit(const Enter& /*node*/) {
		// TODO: check retire
	}

	void visit(const Exit& /*node*/) {
		/* do nothing */
	}


	void visit(const Function& function) {
		if (function.kind == Function::MACRO) {
			raise_error<UnsupportedConstructError>("functions of kind 'MACRO' are not supported in the translation to CAVE lingo");
		} else if (function.kind == Function::SMR) {
			return;
		} else {
			print_delimiter(stream, function.name + "()");
			assert(function.kind == Function::INTERFACE);
			stream << indent << type2cave(function.return_type) << " " << function.name << "(";
			bool first = true;
			for (const auto& decl : function.args) {
				if (!first) {
					stream << ", ";
				}
				print_var_def(*decl, false);
				first = false;
			}
			stream << ") ";
			assert(function.body);
			function.body->accept(*this);
		}
	}

	void visit(const Program& program) {
		stream << "// generated CAVE output for program '" << program.name << "'" << std::endl << std::endl;

		// asserts
		print_delimiter(stream, "assert implementation");
		stream << "void inline fail() {" << std::endl;
		stream << "    Node myNull, myRes;" << std::endl;
		stream << "    myNull = NULL;" << std::endl;
		stream << "    myRes = myNull->tl; // force verification failure" << std::endl;
		stream << "}" << std::endl;

		stream << "void inline assertEq(Node lhs, Node rhs) {" << std::endl;
		stream << "    Node myNull, myRes;" << std::endl;
		stream << "    myNull = NULL;" << std::endl;
		stream << "    if (lhs != rhs) {" << std::endl;
		stream << "        myRes = myNull->tl; // force verification failure" << std::endl;
		stream << "    }" << std::endl << std::endl;

		// TODO: check base observer?
		stream << "void inline assertActive(Node node) {" << std::endl;
		stream << "    Node myNull, myRes;" << std::endl;
		stream << "    myNull = NULL;" << std::endl;
		stream << "    myRes = myNull->tl; // force verification failure" << std::endl;
		stream << "    /* not yet implemented */" << std::endl;
		stream << "}" << std::endl;

		// type defs
		stream << std::endl;
		print_delimiter(stream, "type definitions");
		for (const auto& type : program.types) {
			print_type_def(*type);
		}

		// shared variables
		stream << std::endl;
		print_delimiter(stream, "shared variables");
		stream << "extern int EMPTY;" << std::endl << std::endl;
		for (const auto& decl : program.variables) {
			print_var_def(*decl);
		}

		// initializer
		stream << std::endl;
		print_delimiter(stream, "initializer");
		print_initializer(*program.initializer);

		// functions
		stream << std::endl;
		for (const auto& function : program.functions) {
			function->accept(*this);
		}
	}
};

void prtypes::to_cave_input(const Program& program, const Guarantee& /*active*/, std::ostream& stream) {
	CaveOutputVisitor visitor(stream);
	program.accept(visitor);
	throw std::logic_error("not yet implemented: prtypes::to_cave_input");
}

std::string prtypes::to_cave_input(const Program& program, const Guarantee& active) {
	std::stringstream stream;
	prtypes::to_cave_input(program, active, stream);
	return stream.str();
}

bool prtypes::discharge_assertions(const Program& program, const Guarantee& active) {
	to_cave_input(program, active, std::cout);
	throw std::logic_error("not yet implemented: prtypes::discharge_assertions");
}
