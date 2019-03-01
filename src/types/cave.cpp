#include "types/cave.hpp"
#include "types/guarantees.hpp"
#include "cola/ast.hpp"
#include "cola/util.hpp"
#include "types/error.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <array>

using namespace cola;
using namespace prtypes;

static const bool INSTRUMENT_OBJECTS = false; // does not work
static const bool INSTRUMENT_FLAG = false; // does not work
static const bool INSTRUMENT_WITH_HINT = false; // does not improve precision
static const bool INSTRUMENT_WRAP_ASSERT_ACTIVE = false;
static const bool INSTRUMENT_INSERT_OPTS = true;

// TODO: guard assertActive with != NULL??
// TODO: remove imp var option

void print_delimiter(std::ostream& stream, std::string text) {
	stream << "// ----------------------------------------------------------------------" << std::endl;
	stream << "// " << text << std::endl;
	stream << "// ----------------------------------------------------------------------" << std::endl;
}

const Type& get_retire_type(const Function& retire_function) {
	assert(retire_function.args.size() == 1);
	assert(retire_function.args.at(0)->type.sort == Sort::PTR);
	return retire_function.args.at(0)->type;

}

// TODO: forbid annotated invariants at commands/statements

struct CaveOutputVisitor : public Visitor {
	std::ostream& stream;
	Indent indent;
	const bool instrument;
	const Function* retire;
	const Type* retire_type;
	const std::set<const Assert*>* whitelist;
	bool add_fix_me = false;
	bool needs_parents = false;
	std::set<std::string> opt_keys = { "cave" };

	CaveOutputVisitor(std::ostream& stream, const Function& retire_function, const std::set<const Assert*>* whitelist) : stream(stream), indent(stream), instrument(true), retire(&retire_function), retire_type(&get_retire_type(retire_function)), whitelist(whitelist) {
		assert(!INSTRUMENT_OBJECTS);
		assert(!INSTRUMENT_FLAG);
	}

	CaveOutputVisitor(std::ostream& stream) : stream(stream), indent(stream), instrument(false), retire(nullptr), retire_type(nullptr), whitelist(nullptr) {
		assert(!INSTRUMENT_OBJECTS);
		assert(!INSTRUMENT_FLAG);
	}

    // ********************************************************************* //
	// ****************************** HELPERS ****************************** //
	// ********************************************************************* //

	void print_options(const Program& program) {
		if (INSTRUMENT_INSERT_OPTS) {
			for (std::string key : opt_keys) {
				auto search = program.options.find(key);
				if (search != program.options.end()) {
					stream << indent << search->second << std::endl;
				}
			}
		}
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
		if (this->instrument && INSTRUMENT_OBJECTS) {
			if (&type == retire_type) {
				// instrument retire flag
				stream << std::endl;
				stream << indent << "// instrumented retire flag" << std::endl;
				stream << indent << "bool RETIRED_;" << std::endl;
			}
		}
		indent--;
		stream << "}" << std::endl << std::endl;
	}

	std::string var2cave(const VariableDeclaration& decl) {
		if (decl.is_shared) {
			return "CC_->" + decl.name;
		} else {
			return decl.name;
		}
	}

	void print_instrumentation_decls() {
		if (this->instrument && !INSTRUMENT_OBJECTS) {
			stream << std::endl;
			stream << indent << type2cave(*retire_type) << " INSTRUMENTATION_PTR_;" << std::endl;
			if (INSTRUMENT_WITH_HINT) {
				stream << indent << type2cave(*retire_type) << " HINT_PTR_;" << std::endl;
			}
			if (INSTRUMENT_FLAG) {
				stream << indent << "bool INSTRUMENTATION_RETIRED_;" << std::endl;
			}
		}
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
		stream << indent << "// calling this avoids strange assertion errors in CAVE" << std::endl;
		stream << indent << "atomic {" << std::endl;
		indent++;
		stream << indent << "BUG_FIXER_->field;" << std::endl;
		indent--;
		stream << indent << "}" << std::endl;
		indent--;
		stream << indent << "}" << std::endl;
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
		stream << indent << "CC_ = new();" << std::endl;
		stream << indent;
		assert(init.body);
		init.body->accept(*this);
		stream << std::endl;
		if (this->instrument && !INSTRUMENT_OBJECTS) {
			stream << indent << "// initialize instrumentation" << std::endl;
			stream << indent << "CC_->INSTRUMENTATION_PTR_ = NULL;" << std::endl;
			if (INSTRUMENT_WITH_HINT) {
				stream << indent << "CC_->HINT_PTR_ = NULL;" << std::endl;
			}
			if (INSTRUMENT_FLAG) {
				stream << indent << "CC_->INSTRUMENTATION_RETIRED_ = false;" << std::endl;
			}
			stream << std::endl;
		}
		stream << indent << "// initialize bug fixer" << std::endl;
		stream << indent << "BUG_FIXER_ = new();" << std::endl;
		stream << indent << "BUG_FIXER_->field = 0;" << std::endl;
		indent--;
		stream << indent << "}" << std::endl;
		indent--;
		stream << indent << "}" << std::endl;
	}

	void print_assert_macros() {
		if (this->instrument) {
			stream << indent << "class DummyFailHelper_ {" << std::endl;
			indent++;
			stream << indent << "int field;" << std::endl;
			indent--;
			stream << indent << "}" << std::endl << std::endl;
			stream << indent << "void inline fail() {" << std::endl;
			indent++;
			stream << indent << "DummyFailHelper_ myNull_fail;" << std::endl;
			stream << indent << "int myRes;" << std::endl;
			stream << indent << "myNull_fail = NULL;" << std::endl;
			stream << indent << "myRes = myNull_fail->field; // force verification failure" << std::endl;
			indent--;
			stream << indent << "}" << std::endl << std::endl;

			std::string typeName = type2cave(*retire_type);
			stream << indent << "void inline assertActive(" << typeName << " ptr) {" << std::endl;
			indent++;
			stream << indent << "DummyFailHelper_ myNull_assertActive;" << std::endl;
			stream << indent << "int myRes;" << std::endl;
			stream << indent << "myNull_assertActive = NULL;" << std::endl;
			if (INSTRUMENT_OBJECTS) {
				stream << indent << "if (ptr->RETIRED_) {" << std::endl;
			} else {
				if (INSTRUMENT_FLAG) {
					stream << indent << "if (CC_->INSTRUMENTATION_PTR_ != NULL && CC_->INSTRUMENTATION_RETIRED_ && CC_->INSTRUMENTATION_PTR_ == ptr) {" << std::endl;
				} else {
					stream << indent << "if (CC_->INSTRUMENTATION_PTR_ != NULL && CC_->INSTRUMENTATION_PTR_ == ptr) {" << std::endl;
				}
			}
			indent++;
			stream << indent << "myRes = myNull_assertActive->field; // force verification failure" << std::endl;
			indent--;
			stream << indent << "}" << std::endl;
			indent--;
			stream << indent << "}" << std::endl;
		}
	}

	// ********************************************************************* //
	// **************************** EXPRESSIONS **************************** //
	// ********************************************************************* //

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

	void visit(const Dereference& expr) {
		stream << "(";
		assert(expr.expr);
		expr.expr->accept(*this);
		stream << ")->" << expr.fieldname;
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

	// ********************************************************************* //
	// ******************************* BLOCKS ****************************** //
	// ********************************************************************* //

	void visit(const Scope& scope) {
		stream << "{" << std::endl;
		indent++;
		for (const auto& decl : scope.variables) {
			print_var_def(*decl);
		}
		if (INSTRUMENT_WITH_HINT && add_fix_me) {
			stream << indent << "bool HINT_PROPHECY_;" << std::endl;
			stream << indent << "HINT_PROPHECY_ = false;" << std::endl << std::endl;
		} else if (!scope.variables.empty()) {
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

	void visit(const IfThenElse& node) {
		stream << indent << "if ( ";
		node.expr->accept(*this);
		stream << ") ";
		node.ifBranch->accept(*this);
		stream << indent << "else ";
		node.elseBranch->accept(*this);
	}
	void visit(const While& node) {
		stream << indent << "while (";
		assert(node.expr);
		node.expr->accept(*this);
		stream << ") ";
		assert(node.body);
		node.body->accept(*this);
	}

	// ********************************************************************* //
	// ****************************** COMMANDS ***************************** //
	// ********************************************************************* //

	void visit(const Break& /*node*/) {
		stream << indent << "break;" << std::endl;
		// raise_error<UnsupportedConstructError>("'break' not supported in the translation to CAVE");
	}
	
	void visit(const Continue& /*node*/) {
		raise_error<UnsupportedConstructError>("'continue' not supported in the translation to CAVE");
	}
	
	void visit(const Return& node) {
		stream << indent << "return ";
		node.expr->accept(*this);
		stream << ";" << std::endl;
	}
	
	void visit(const Macro& /*node*/) { 
		raise_error<UnsupportedConstructError>("calls to macros are not supported in the translation to CAVE");
	}

	void visit(const CompareAndSwap& /*node*/) {
		raise_error<UnsupportedConstructError>("'CAS' is not supported in the translation to CAVE");
	}

	void visit(const Skip& /*node*/) {
		/* do nothing */
		stream << indent << "// skip;" << std::endl;
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
		if (INSTRUMENT_WRAP_ASSERT_ACTIVE) {
			stream << indent << "if (";
			invariant.expr->accept(*this);
			stream << " != NULL) { ";
		} else {
			stream << indent;
		}
		stream << "assertActive(";
		assert(invariant.expr);
		invariant.expr->accept(*this);
		stream << ");";
		if (INSTRUMENT_WRAP_ASSERT_ACTIVE) {
			stream << " }";
		}
		stream << std::endl;
	}

	void visit(const Assert& assrt) {
		if (this->instrument) {
			if (!this->whitelist || whitelist->count(&assrt) > 0) {
				assert(assrt.inv);
				assrt.inv->accept(*this);
			}
		}
	}

	void visit(const Malloc& malloc) {
		stream << indent << var2cave(malloc.lhs) << " = new();" << std::endl;
		if (this->instrument && &malloc.lhs.type == retire_type) {
			if (INSTRUMENT_OBJECTS) {
				stream << indent << var2cave(malloc.lhs) << "->RETIRED_ = false;" << std::endl;
			} else if (INSTRUMENT_FLAG) {
				stream << indent << "atomic { if (CC_->INSTRUMENTATION_PTR_ == NULL) { if (*) { CC_->INSTRUMENTATION_PTR_ = " << var2cave(malloc.lhs) << "; } } }" << std::endl;
			}
		}
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
		stream << indent << "// ";
		cola::print(enter, stream);
		if (this->instrument && INSTRUMENT_WITH_HINT && enter.decl.name == "caveHintRetire") {
			assert(!INSTRUMENT_OBJECTS);
			assert(enter.args.size() == 1);
			assert(enter.args.at(0)->type().sort == Sort::PTR);
			assert(retire->args.size() == 1);
			assert(&retire->args.at(0)->type == &enter.args.at(0)->type());
			stream << indent << "if (";
			enter.args.at(0)->accept(*this);
			stream << " == NULL) { fail(); }" << std::endl;
			stream << indent << "if (HINT_PROPHECY_) { fail(); }" << std::endl;;
			stream << indent << "if (CC_->HINT_PTR_ == NULL && CC_->INSTRUMENTATION_PTR_ == NULL) { if (*) { CC_->HINT_PTR_ = ";
			enter.args.at(0)->accept(*this);
			stream << "; HINT_PROPHECY_ = true; } }" << std::endl;
		}
		if (this->instrument && &enter.decl == retire) {
			assert(enter.args.size() == 1);
			assert(enter.args.at(0)->type().sort == Sort::PTR);

			// assert that NULL is not retired
			stream << indent << "if (";
			enter.args.at(0)->accept(*this);
			stream << " == NULL) { fail(); }" << std::endl;

			if (INSTRUMENT_OBJECTS) {
				// instrumentation: set retired flag
				stream << indent << "(";
				enter.args.at(0)->accept(*this);
				stream << ")->RETIRED_ = true;" << std::endl;

			} else if (!INSTRUMENT_FLAG) {
				if (INSTRUMENT_WITH_HINT) {
					stream << indent << "atomic { if (HINT_PROPHECY_) {" << std::endl;
					indent++;
					stream << indent << "HINT_PROPHECY_ = false;" << std::endl;
					stream << indent << "if (CC_->HINT_PTR_ != ";
					enter.args.at(0)->accept(*this);
					stream << ") { fail(); }" << std::endl;
					stream << indent << "if (CC_->HINT_PTR_ == NULL) { fail(); }" << std::endl;
					stream << indent << "if (CC_->INSTRUMENTATION_PTR_ != NULL) { fail(); }" << std::endl;
					stream << indent << "CC_->INSTRUMENTATION_PTR_ = CC_->HINT_PTR_;" << std::endl;
					indent--;
					stream << indent << "}}" << std::endl;
				} else {
					// instrumentation: set if not yet set
					stream << indent << "if (CC_->INSTRUMENTATION_PTR_ == NULL) { if (*) { CC_->INSTRUMENTATION_PTR_ = ";
					enter.args.at(0)->accept(*this);
					stream << "; } }" << std::endl;
				}
			}
		}
	}

	void visit(const Exit& exit) {
		/* do nothing */
		stream << indent << "// ";
		print(exit, stream);
	}

	// ********************************************************************* //
	// ************************ FUNCTIONS / PROGRAMS *********************** //
	// ********************************************************************* //

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
		stream << "// generated CAVE program for '" << program.name << "'" << std::endl;
		print_options(program);
		stream << std::endl;

		// type defs
		print_delimiter(stream, "type definitions");
		for (const auto& type : program.types) {
			print_type_def(*type);
		}

		// shared variables
		print_delimiter(stream, "shared variables");
		stream << indent << "extern int EMPTY;" << std::endl << std::endl;
		stream << indent << "class Container_ {" << std::endl;
		indent++;
		for (const auto& decl : program.variables) {
			print_var_def(*decl);
		}
		print_instrumentation_decls();
		indent--;
		stream << indent << "}" << std::endl << std::endl;
		stream << indent << "Container_ CC_;" << std::endl << std::endl;

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

void to_cave_input(const Program& program, const Function& retire_function, const Guarantee& /*active*/, std::set<const Assert*>* whitelist, std::ostream& stream) {
	CaveOutputVisitor visitor(stream, retire_function, whitelist);
	program.accept(visitor);
}

/**
 * See: https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-output-of-command-within-c-using-posix
 */
std::string exec(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}


bool discharge_assertions_impl(const Program& program, const Function& retire_function, const Guarantee& active, std::set<const Assert*>* whitelist) {
	std::string filename = "tmp.cav";
	std::ofstream outfile(filename);
	to_cave_input(program, retire_function, active, whitelist, outfile);
	outfile.close();

	// std::string command = "./cave -allow_leaks -lm -por " + filename;
	std::string command = "./cave -allow_leaks " + filename;
	std::string result = exec(command.data());
	if (result.find("\nNOT Valid\n") != std::string::npos) {
		return false;
	} else if (result.find("\nValid\n") != std::string::npos) {
		return true;
	} else {
		throw CaveError("CAVE failed me (unrcognized output)! Cannot recover.");
	}
}

bool prtypes::discharge_assertions(const Program& program, const Function& retire_function, const Guarantee& active) {
	return discharge_assertions_impl(program, retire_function, active, nullptr);
}


bool prtypes::discharge_assertions(const Program& program, const Function& retire_function, const Guarantee& active, const std::vector<std::reference_wrapper<const Assert>>& whitelist) {
	std::set<const Assert*> set;
	for (const Assert& assert : whitelist) {
		set.insert(&assert);
	}
	return discharge_assertions_impl(program, retire_function, active, &set);
}


bool prtypes::check_linearizability(const cola::Program& program) {
	std::string filename = "tmp.cav";
	std::ofstream outfile(filename);
	CaveOutputVisitor visitor(outfile);
	visitor.opt_keys.insert("cavelin");
	program.accept(visitor);
	outfile.close();

	std::string spec_file = "spec.cav";
	auto find = program.options.find("cavespec");
	if (find != program.options.end()) {
		// TODO: relative path from executable
		spec_file = find->second;
	}
	find = program.options.find("_path");
	if (find != program.options.end()) {
		spec_file = find->second + spec_file;
	}

	std::string command = "./cave -linear " + spec_file + " " + filename;
	std::string result = exec(command.data());
	if (result.find("\nNOT Valid\n") != std::string::npos) {
		return false;
	} else if (result.find("\nValid\n") != std::string::npos) {
		return true;
	} else {
		throw CaveError("CAVE failed me (unrcognized output)! Cannot recover.");
	}
}










