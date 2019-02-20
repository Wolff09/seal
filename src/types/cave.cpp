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

struct CheckAtomicVisitor : public Visitor {
	bool contains_malloc = false;
	bool only_malloc = true;
	bool only_data = true;
	void visit(const VariableDeclaration& node) {
		if (node.type.sort != Sort::DATA && node.type.sort != Sort::BOOL) {
			only_data = false;
		}
	}
	void visit(const Expression& /*node*/) { throw std::logic_error("Unexpected invocation: CheckAtomicVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) { /* do nothing */ }
	void visit(const NullValue& /*node*/) { only_data = false; }
	void visit(const EmptyValue& /*node*/) { /* do nothing */ }
	void visit(const NDetValue& /*node*/) { /* do nothing */ }
	void visit(const VariableExpression& node) {
		node.decl.accept(*this);
	}
	void visit(const NegatedExpression& node) {
		node.expr->accept(*this);
	}
	void visit(const BinaryExpression& node) {
		node.lhs->accept(*this);
		node.rhs->accept(*this);
	}
	void visit(const Dereference& /*node*/) { only_data = false; }
	void visit(const InvariantExpression& /*node*/) { only_data = false; }
	void visit(const InvariantActive& /*node*/) { only_data = false; }
	void visit(const Function& /*node*/) { throw std::logic_error("Unexpected invocation: CheckAtomicVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) { throw std::logic_error("Unexpected invocation: CheckAtomicVisitor::visit(const Program&)"); }
	void visit(const Sequence& node) {
		only_malloc = false;
		node.first->accept(*this);
		node.second->accept(*this);
	}
	void visit(const Scope& node) {
		only_malloc = false;
		node.body->accept(*this);
	}
	void visit(const Atomic& node) {
		node.body->body->accept(*this);
	}
	void visit(const Choice& node) {
		only_malloc = false;
		for (const auto& branch : node.branches) {
			branch->accept(*this);
		}
	}
	void visit(const IfThenElse& node) {
		only_malloc = false;
		node.expr->accept(*this);
		node.ifBranch->accept(*this);
		node.elseBranch->accept(*this);
	}
	void visit(const Loop& node) {
		only_malloc = false;
		node.body->accept(*this);
	}
	void visit(const While& node) {
		only_malloc = false;
		node.body->accept(*this);
	}
	void visit(const Skip& /*node*/) { /* do nothing */ }
	void visit(const Break& /*node*/) { only_malloc = false; }
	void visit(const Continue& /*node*/) { only_malloc = false; }
	void visit(const Assume& node) {
		node.expr->accept(*this);
		only_malloc = false;
	}
	void visit(const Assert& node) {
		node.inv->accept(*this);
		only_malloc = false;
	}
	void visit(const Return& node) {
		node.expr->accept(*this);
		only_malloc = false;
	}
	void visit(const Assignment& node) {
		node.lhs->accept(*this);
		node.rhs->accept(*this);
		only_malloc = false;
	}
	void visit(const Enter& /*node*/) { only_malloc = false; only_data = false; }
	void visit(const Exit& /*node*/) { only_malloc = false; only_data = false; }
	void visit(const Macro& /*node*/) { only_malloc = false; only_data = false; }
	void visit(const CompareAndSwap& /*node*/) { only_malloc = false; only_data = false; }
	void visit(const Malloc& /*node*/) { contains_malloc = true; only_data = false; }
};

// TODO: forbid annotated invariants at commands/statements

struct CaveOutputVisitor : public Visitor {
	std::ostream& stream;
	Indent indent;
	const Function& retire;
	bool add_fix_me = false;

	CaveOutputVisitor(std::ostream& stream, const Function& retire_function) : stream(stream), indent(stream), retire(retire_function) {
		assert(retire.args.size() == 1);
		assert(retire.args.at(0)->type.sort == Sort::PTR);
	}

	std::string type2cave(const Type& type) {
		if (type.sort == Sort::DATA) {
			return "int";
		} else if (type.sort == Sort::PTR) {
			// prune trailing *
			assert(type.name.back() == '*');
			std::string result = type.name;
			result.pop_back();
			return result;
		} else {
			return type.name;
		}
	}

	void print_type_def(const Type& type) {
		stream << "class " << type2cave(type) << " {" << std::endl;
		indent++;
		for (const auto& [fiedl_name, type] : type.fields) {
			stream << indent << type2cave(type) << " " << fiedl_name << ";" << std::endl;
		}
		indent--;
		stream << "}" << std::endl << std::endl;
	}

	std::string var2cave(const VariableDeclaration& decl) {
		// if (decl.is_shared) {
		// 	return "C_->" + decl.name;
		// } else {
		// 	return decl.name;
		// }
		return decl.name;
	}

	void print_instrumentation_decls() {
		stream << indent << type2cave(retire.args.at(0)->type) << " INSTRUMENTATION_PTR_;" << std::endl;
		stream << indent << type2cave(retire.args.at(0)->type) << " INSTRUMENTATION_RETIRED_;" << std::endl;
	}

	void print_var_def(const VariableDeclaration& decl, bool with_semicolon=true) {
		if (with_semicolon) {
			stream << indent << type2cave(decl.type) << " " << decl.name << ";" << std::endl;
		} else {
			stream << indent << type2cave(decl.type) << " " << decl.name;
		}
	}

	void print_bug_fixer() {
		stream << indent << "class BugFixerHelper_ {" << std::endl;
		indent++;
		stream << indent << "int field;" << std::endl;
		indent--;
		stream << indent << "}" << std::endl << std::endl;
		stream << indent << "BugFixerHelper_ BUG_FIXER_;" << std::endl << std::endl;
		stream << indent << "void inline fix_me_() {" << std::endl;
		indent++;
		stream << indent << "atomic {" << std::endl;
		indent++;
		stream << indent << "BUG_FIXER_->field;" << std::endl;
		indent--;
		stream << "}" << std::endl;
		indent--;
		stream << "}" << std::endl;
	}

	void print_bug_fixer_call() {
		stream << indent << "fix_me_();" << std::endl;
	}

	void print_initializer(const Function& init) {
		stream << "resource r() {" << std::endl;
		indent++;
		stream << indent << "constructor {" << std::endl;
		indent++;
		stream << indent << "// initialize container" << std::endl;
		stream << indent;
		assert(init.body);
		init.body->accept(*this);
		stream << std::endl;
		stream << indent << "// initialize instrumentation" << std::endl;
		stream << indent << "INSTRUMENTATION_PTR_ = NULL;" << std::endl;
		stream << indent << "INSTRUMENTATION_RETIRED_ = false;" << std::endl;
		stream << indent << "BUG_FIXER_ = new()" << std::endl;
		stream << indent << "BUG_FIXER_->field = 0;" << std::endl;
		indent--;
		stream << indent << "}" << std::endl;
		indent--;
		stream << indent << "}" << std::endl;
	}

	void print_assert_macros() {
		stream << indent << "class DummyFailHelper_ {" << std::endl;
		indent++;
		stream << indent << "int field;" << std::endl;
		indent--;
		stream << indent << "}" << std::endl << std::endl;
		stream << indent << "void inline fail() {" << std::endl;
		indent++;
		stream << indent << "DummyFailHelper_ myFailNull;" << std::endl;
		stream << indent << "int myRes;" << std::endl;
		stream << indent << "myFailNull = NULL;" << std::endl;
		stream << indent << "myRes = myFailNull->field; // force verification failure" << std::endl;
		indent--;
		stream << indent << "}" << std::endl << std::endl;

		std::string typeName = type2cave(retire.args.at(0)->type);
		stream << indent << "void inline assertActive(" << typeName << " ptr) {" << std::endl;
		indent++;
		stream << indent << "DummyFailHelper_ myAssertNull;" << std::endl;
		stream << indent << "int myRes;" << std::endl;
		stream << indent << "myAssertNull = NULL;" << std::endl;
		stream << indent << "if (INSTRUMENTATION_RETIRED_ && INSTRUMENTATION_PTR_ == ptr) {" << std::endl;
		indent++;
		stream << indent << "myRes = myAssertNull->field; // force verification failure" << std::endl;
		indent--;
		stream << indent << "}" << std::endl;
		indent--;
		stream << indent << "}" << std::endl;
	}

	void visit(const Expression& /*expr*/) { throw std::logic_error("Unexpected invocation: CaveOutputVisitor::visit(const Expression&)"); }	

	void visit(const VariableDeclaration& expr) {
		stream << var2cave(expr);
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
		stream << var2cave(expr.decl);
	}

	void visit(const NegatedExpression& expr) {
		stream << "!(";
		assert(expr.expr);
		expr.expr->accept(*this);
		stream << ")";
	}

	void visit(const BinaryExpression& expr) {
		assert(expr.lhs);
		assert(expr.rhs);
		stream << "(";
		expr.lhs->accept(*this);
		stream << ")";
		switch (expr.op) {
			case BinaryExpression::Operator::EQ: stream << " == "; break;
			case BinaryExpression::Operator::NEQ: stream << " != "; break;
			case BinaryExpression::Operator::LEQ: stream << " <= "; break;
			case BinaryExpression::Operator::LT: stream << " < "; break;
			case BinaryExpression::Operator::GEQ: stream << " >= "; break;
			case BinaryExpression::Operator::GT: stream << " > "; break;
			case BinaryExpression::Operator::AND: stream << " && "; break;
			case BinaryExpression::Operator::OR: stream << " || "; break;
		}
		stream << "(";
		expr.rhs->accept(*this);
		stream << ")";
	}

	void visit(const Dereference& expr) {
		stream << "(";
		assert(expr.expr);
		expr.expr->accept(*this);
		stream << ")->" << expr.fieldname;
	}
	
	void visit(const Scope& scope) {
		stream << "{" << std::endl;
		indent++;
		for (const auto& decl : scope.variables) {
			print_var_def(*decl);
		}
		if (!scope.variables.empty()) {
			stream << std::endl;
		}
		if (add_fix_me) {
			print_bug_fixer_call();
			add_fix_me = false;
		}
		assert(scope.body);
		scope.body->accept(*this);
		indent--;
		stream << indent << "}" << std::endl;
	}

	void visit(const Atomic& atomic) {
		CheckAtomicVisitor visitor;
		atomic.accept(visitor);
		assert(!(visitor.contains_malloc && !visitor.only_malloc));

		// CAVE does not want malloc / new() inside an atomic block
		// CAVE sometimes (probably only Viktor knows when) does not want data inside an atomic block
		if (visitor.contains_malloc || visitor.only_data) {
			assert(visitor.contains_malloc ^ visitor.only_data);
			conditionally_raise_error<UnsupportedConstructError>(visitor.contains_malloc && !visitor.only_malloc, "'malloc' must appear alone in atomic block for the translation to CAVE");
			assert(atomic.body);
			assert(atomic.body->body);
			atomic.body->body->accept(*this);
		} else {
			stream << indent << "atomic ";
			assert(atomic.body);
			atomic.body->accept(*this);	
		}
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
		assert(choice.branches.size() <= 2);
		bool first = true;
		for (const auto& branch : choice.branches) {
			if (!first) {
				stream << indent << "else ";
			}
			assert(branch);
			branch->accept(*this);
			first = false;
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
		assume.expr->accept(*this);
		stream << ");" << std::endl;
	}

	void visit(const InvariantExpression& invariant) {
		stream << indent << "if (!(";
		assert(invariant.expr);
		invariant.expr->accept(*this);
		stream << ")) { fail(); }" << std::endl;
	}
	void visit(const InvariantActive& invariant) {
		// TODO: invoke macro only if non null <<<<<<<<---------------------------------------------------------------------------------------|||||||||||||||||||||||||
		stream << indent << "assertActive(";
		assert(invariant.expr);
		invariant.expr->accept(*this);
		stream << ");" << std::endl;
	}

	void visit(const Assert& assrt) {
		assert(assrt.inv);
		assrt.inv->accept(*this);
	}

	void visit(const Malloc& malloc) {
		stream << indent << var2cave(malloc.lhs) << " = new();" << std::endl;
	}

	void visit(const Assignment& assign) {
		stream << indent;
		assert(assign.lhs);
		assign.lhs->accept(*this);
		stream << " = ";
		assert(assign.rhs);
		assign.rhs->accept(*this);
		stream << ";" << std::endl;
	}

	void visit(const Enter& enter) {
		if (&enter.decl == &retire) {
			assert(enter.args.size() == 1);
			assert(enter.args.at(0)->type().sort == Sort::PTR);

			// assert that NULL is not retired
			stream << indent << "if (";
			enter.args.at(0)->accept(*this);
			stream << " == NULL) { fail(); }" << std::endl;

			// instrumentation: set if not yet set
			stream << indent << "if (INSTRUMENTATION_PTR_ == NULL) { if (*) { INSTRUMENTATION_PTR_ = ";
			enter.args.at(0)->accept(*this);
			stream << "; } }" << std::endl;

			// instrumentation: update if needed
			stream << indent << "if (INSTRUMENTATION_PTR_ == ";
			enter.args.at(0)->accept(*this);
			stream << ") { INSTRUMENTATION_RETIRED_ = true; }" << std::endl;
		}
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
			add_fix_me = true;
			function.body->accept(*this);
			add_fix_me = false;
			stream << std::endl;
		}
	}

	void visit(const Program& program) {
		stream << "// generated CAVE output for program '" << program.name << "'" << std::endl << std::endl;
		// stream << "prover_opts imp_var = \"C_.Head.next\";" << std::endl << std::endl; // TODO: do not hardcode this!

		// type defs
		print_delimiter(stream, "type definitions");
		for (const auto& type : program.types) {
			print_type_def(*type);
		}

		// shared variables
		print_delimiter(stream, "shared variables");
		stream << indent << "extern int EMPTY;" << std::endl << std::endl;
		for (const auto& decl : program.variables) {
			print_var_def(*decl);
		}
		stream << std::endl;
		print_instrumentation_decls();
		stream << std::endl;

		// fix Cave strangeness
		print_delimiter(stream, "bug fixer");
		print_bug_fixer();

		// assertion macros
		stream << std::endl;
		print_delimiter(stream, "assert implementation");
		print_assert_macros();

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

void prtypes::to_cave_input(const Program& program, const Function& retire_function, const Guarantee& /*active*/, std::ostream& stream) {
	CaveOutputVisitor visitor(stream, retire_function);
	program.accept(visitor);
	throw std::logic_error("not yet implemented: prtypes::to_cave_input");
}

std::string prtypes::to_cave_input(const Program& program, const Function& retire_function, const Guarantee& active) {
	std::stringstream stream;
	prtypes::to_cave_input(program, retire_function, active, stream);
	return stream.str();
}

bool prtypes::discharge_assertions(const Program& program, const Function& retire_function, const Guarantee& active) {
	to_cave_input(program, retire_function, active, std::cout);
	throw std::logic_error("not yet implemented: prtypes::discharge_assertions");
}
