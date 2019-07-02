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


struct DerefAssignmentVisitor final : public Visitor {
	bool deref_has_begun = false;
	bool deref_has_ended = false;
	bool expr_is_deref = false;
	std::vector<const Assignment*> deref_assignments;

	void visit(const InvariantExpression& /*node*/) { throw std::logic_error("Unexpected invocation: DerefAssignmentVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) { throw std::logic_error("Unexpected invocation: DerefAssignmentVisitor::visit(const InvariantActive&)"); }
	void visit(const Expression& /*node*/) { throw std::logic_error("Unexpected invocation: DerefAssignmentVisitor::visit(const Expression&)"); }
	void visit(const VariableDeclaration& /*node*/) { this->expr_is_deref = false; }
	void visit(const BooleanValue& /*node*/) { this->expr_is_deref = false; }
	void visit(const NullValue& /*node*/) { this->expr_is_deref = false; }
	void visit(const EmptyValue& /*node*/) { this->expr_is_deref = false; }
	void visit(const MaxValue& /*node*/) { this->expr_is_deref = false; }
	void visit(const MinValue& /*node*/) { this->expr_is_deref = false; }
	void visit(const NDetValue& /*node*/) { this->expr_is_deref = false; }
	void visit(const VariableExpression& /*node*/) { this->expr_is_deref = false; }
	void visit(const NegatedExpression& /*node*/) { this->expr_is_deref = false; }
	void visit(const BinaryExpression& /*node*/) { this->expr_is_deref = false; }
	void visit(const Dereference& /*node*/) { this->expr_is_deref = true; }

	void visit(const Sequence& node) {
		node.first->accept(*this);
		node.second->accept(*this);
	}
	void visit(const Scope& node) {
		deref_has_ended = deref_has_begun;
		node.body->accept(*this);
		deref_has_ended = deref_has_begun;
	}
	void visit(const Atomic& node) {
		deref_has_ended = deref_has_begun;
		node.body->accept(*this);
		deref_has_ended = deref_has_begun;
	}
	void visit(const Choice& node) {
		for (const auto& branch : node.branches) {
			deref_has_ended = deref_has_begun;
			branch->accept(*this);
			deref_has_ended = deref_has_begun;
		}
	}
	void visit(const IfThenElse& node) {
		deref_has_ended = deref_has_begun;
		node.ifBranch->accept(*this);
		deref_has_ended = deref_has_begun;
		node.elseBranch->accept(*this);
		deref_has_ended = deref_has_begun;
	}
	void visit(const Loop& node) {
		deref_has_ended = deref_has_begun;
		node.body->accept(*this);
		deref_has_ended = deref_has_begun;
	}
	void visit(const While& node) {
		deref_has_ended = deref_has_begun;
		node.body->accept(*this);
		deref_has_ended = deref_has_begun;
	}

	void visit(const Assignment& node) {
		node.lhs->accept(*this);
		if (expr_is_deref) {
			conditionally_raise_error<UnsupportedConstructError>(deref_has_ended, "assignments to dereference expressions must appear subsequentially within an atomic block");
			deref_has_begun = true;
			deref_assignments.push_back(&node);
		} else {
			deref_has_ended = deref_has_begun;
		}
	}
	void visit(const Skip& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const Break& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const Continue& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const Assume& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const Assert& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const AngelChoose& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const AngelActive& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const AngelContains& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const Return& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const Malloc& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const Enter& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const Exit& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const Macro& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const CompareAndSwap& /*node*/) { deref_has_ended = deref_has_begun; }
	void visit(const Function& /*node*/) { throw std::logic_error("Unexpected invocation: DerefAssignmentVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) { throw std::logic_error("Unexpected invocation: DerefAssignmentVisitor::visit(const Program&)"); }
};


struct CaveConfig {
	bool INSTRUMENT_OBJECTS = true;
	const bool INSTRUMENT_FLAG = false; // not supported (does not work)
	const bool INSTRUMENT_WITH_HINT = false; // not supported (does not improve precision anyways)
	bool INSTRUMENT_WRAP_ASSERT_ACTIVE = false;
	bool INSTRUMENT_INSERT_OPTS = true;
	bool INSTRUMENT_WRAP_SHARED = true;
	std::string MAX_VAL = "12345";
};

void update_conf(CaveConfig& conf, const Program& program) {
	auto search = program.options.find("instrumentation");
	if (search != program.options.end()) {
		std::string opt_val = search->second;
		if (opt_val == "object") {
			conf.INSTRUMENT_OBJECTS = true;
		} else if (opt_val == "container") {
			conf.INSTRUMENT_OBJECTS = false;
		} else {
			throw std::logic_error("Unexpected option parameter: option 'instrumentation' must be 'object' or 'container'.");
		}
	}
}

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
	CaveConfig conf;
	std::ostream& stream;
	Indent indent;
	const bool instrument;
	const Function* retire;
	const Type* retire_type;
	const std::set<const Assert*>* whitelist;
	bool add_fix_me = false;
	bool needs_parents = false;
	const Program* program;
	std::set<std::string> opt_keys = { "cave" };
	bool inside_atomic = false;
	std::vector<const Assignment*> derefs_in_current_atomic;

	CaveOutputVisitor(std::ostream& stream, const Function& retire_function, const std::set<const Assert*>* whitelist) : stream(stream), indent(stream), instrument(true), retire(&retire_function), retire_type(&get_retire_type(retire_function)), whitelist(whitelist) {
	}

	CaveOutputVisitor(std::ostream& stream) : stream(stream), indent(stream), instrument(false), retire(nullptr), retire_type(nullptr), whitelist(nullptr) {
	}

    // ********************************************************************* //
	// ****************************** HELPERS ****************************** //
	// ********************************************************************* //

	void print_options(const Program& program) {
		if (this->conf.INSTRUMENT_INSERT_OPTS) {
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
		if (this->instrument && this->conf.INSTRUMENT_OBJECTS) {
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
		if (this->conf.INSTRUMENT_WRAP_SHARED && decl.is_shared) {
			return "CC_->" + decl.name;
		} else {
			return decl.name;
		}
	}

	void print_instrumentation_decls() {
		if (this->instrument && !this->conf.INSTRUMENT_OBJECTS) {
			stream << std::endl;
			stream << indent << type2cave(*retire_type) << " INSTRUMENTATION_PTR_;" << std::endl;
			if (this->conf.INSTRUMENT_WITH_HINT) {
				stream << indent << type2cave(*retire_type) << " HINT_PTR_;" << std::endl;
			}
			if (this->conf.INSTRUMENT_FLAG) {
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
		if (this->conf.INSTRUMENT_WRAP_SHARED) {
			stream << indent << "CC_ = new();" << std::endl;
		}
		stream << indent;
		assert(init.body);
		init.body->accept(*this);
		stream << std::endl;
		if (this->instrument && !this->conf.INSTRUMENT_OBJECTS) {
			std::string prefix = this->conf.INSTRUMENT_WRAP_SHARED ? "CC_->" : "";
			stream << indent << "// initialize instrumentation" << std::endl;
			stream << indent << prefix << "INSTRUMENTATION_PTR_ = NULL;" << std::endl;
			if (this->conf.INSTRUMENT_WITH_HINT) {
				stream << indent << prefix << "HINT_PTR_ = NULL;" << std::endl;
			}
			if (this->conf.INSTRUMENT_FLAG) {
				stream << indent << prefix << "INSTRUMENTATION_RETIRED_ = false;" << std::endl;
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
			if (this->conf.INSTRUMENT_OBJECTS) {
				stream << indent << "if (ptr->RETIRED_) {" << std::endl;
			} else {
				std::string prefix = this->conf.INSTRUMENT_WRAP_SHARED ? "CC_->" : "";
				if (this->conf.INSTRUMENT_FLAG) {
					stream << indent << "if (" << prefix << "INSTRUMENTATION_PTR_ != NULL && " << prefix << "INSTRUMENTATION_RETIRED_ && " << prefix << "INSTRUMENTATION_PTR_ == ptr) {" << std::endl;
				} else {
					stream << indent << "if (" << prefix << "INSTRUMENTATION_PTR_ != NULL && " << prefix << "INSTRUMENTATION_PTR_ == ptr) {" << std::endl;
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

	void print_requires(const Function& function) {
		assert(this->program);
		auto search = this->program->options.find("cavebound");
		if (search != program->options.end()) {
			std::string name = search->second;
			for (const auto& decl : function.args) {
				if (decl->name == name) {
					stream << "requires -" << this->conf.MAX_VAL << " < " << name << " && " << name << " < " << this->conf.MAX_VAL << " ";
				}
			}
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

	void visit(const MaxValue& /*expr*/) {
		stream << this->conf.MAX_VAL;
	}

	void visit(const MinValue& /*expr*/) {
		stream << "-" << this->conf.MAX_VAL;
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
		if (this->conf.INSTRUMENT_WITH_HINT && add_fix_me) {
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
		conditionally_raise_error<UnsupportedConstructError>(this->inside_atomic, "nested atomics are not supported in the translation to CAVE");
		DerefAssignmentVisitor visitor;
		atomic.accept(visitor);
		this->derefs_in_current_atomic = std::move(visitor.deref_assignments);
		this->inside_atomic = true;

		stream << indent << "atomic ";
		assert(atomic.body);
		atomic.body->accept(*this);

		this->derefs_in_current_atomic.clear();
		this->inside_atomic = false;
	}

	void visit(const Sequence& seq) {
		assert(seq.first);
		assert(seq.second);
		seq.first->accept(*this);
		seq.second->accept(*this);
	}

	void print_choice(const Choice& choice, std::size_t first_branch) {
		assert(choice.branches.size() - first_branch > 0);
		auto remaining_branches = choice.branches.size() - first_branch;
		stream << indent << "if (*) ";
		choice.branches.at(first_branch)->accept(*this);
		remaining_branches--;
		first_branch++;
		if (remaining_branches == 0) {
			return;
		} else if (remaining_branches == 1) {
			stream << indent << "else ";
			choice.branches.at(first_branch)->accept(*this);
		} else {
			stream << indent << "else {" << std::endl;
			indent++;
			print_choice(choice, first_branch);
			indent--;
			stream << indent << "}" << std::endl;
		}
	}

	void visit(const Choice& choice) {
		assert(choice.branches.size() > 0);
		print_choice(choice, 0);
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
		if (this->conf.INSTRUMENT_WRAP_ASSERT_ACTIVE) {
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
		if (this->conf.INSTRUMENT_WRAP_ASSERT_ACTIVE) {
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

	void visit(const AngelChoose& /*angel*/) { throw std::logic_error("CAVE INSTRUMENTATION FOR ANGELS NOT YET IMPLEMENTED"); }
	void visit(const AngelActive& /*angel*/) { throw std::logic_error("CAVE INSTRUMENTATION FOR ANGELS NOT YET IMPLEMENTED"); }
	void visit(const AngelContains& /*angel*/) { throw std::logic_error("CAVE INSTRUMENTATION FOR ANGELS NOT YET IMPLEMENTED"); }

	void visit(const Malloc& malloc) {
		stream << indent << var2cave(malloc.lhs) << " = new();" << std::endl;
		if (this->instrument && &malloc.lhs.type == retire_type) {
			if (this->conf.INSTRUMENT_OBJECTS) {
				stream << indent << var2cave(malloc.lhs) << "->RETIRED_ = false;" << std::endl;
			} else if (this->conf.INSTRUMENT_FLAG) {
				std::string prefix = this->conf.INSTRUMENT_WRAP_SHARED ? "CC_->" : "";
				stream << indent << "atomic { if (" << prefix << "INSTRUMENTATION_PTR_ == NULL) { if (*) { " << prefix << "INSTRUMENTATION_PTR_ = " << var2cave(malloc.lhs) << "; } } }" << std::endl;
			}
		}
	}

	void visit(const Assignment& assign) {
		auto handle_assign = [&] (const Assignment& to_handle) {
			assert(to_handle.lhs);
			to_handle.lhs->accept(*this);
			stream << " = ";
			assert(to_handle.rhs);
			to_handle.rhs->accept(*this);
		};

		if (this->inside_atomic && std::count(this->derefs_in_current_atomic.begin(), this->derefs_in_current_atomic.end(), &assign) > 0) {
			assert(!this->derefs_in_current_atomic.empty());
			if (this->derefs_in_current_atomic.at(0) == &assign) {
				stream << indent;
				bool is_first = true;
				for (const auto& stmt : this->derefs_in_current_atomic) {
					if (!is_first) {
						stream << ", ";
					}
					is_first = false;
					handle_assign(*stmt);
				}
				stream << ";" << std::endl;
			}
		} else {
			stream << indent;
			handle_assign(assign);
			stream << ";" << std::endl;
		}
	}

	void visit(const Enter& enter) {
		stream << indent << "// ";
		cola::print(enter, stream);
		if (this->instrument && this->conf.INSTRUMENT_WITH_HINT && enter.decl.name == "caveHintRetire") {
			assert(!this->conf.INSTRUMENT_OBJECTS);
			assert(enter.args.size() == 1);
			assert(enter.args.at(0)->type().sort == Sort::PTR);
			assert(retire->args.size() == 1);
			assert(&retire->args.at(0)->type == &enter.args.at(0)->type());
			stream << indent << "if (";
			enter.args.at(0)->accept(*this);
			stream << " == NULL) { fail(); }" << std::endl;
			std::string prefix = this->conf.INSTRUMENT_WRAP_SHARED ? "CC_->" : "";
			stream << indent << "if (HINT_PROPHECY_) { fail(); }" << std::endl;;
			stream << indent << "if (" << prefix << "HINT_PTR_ == NULL && " << prefix << "INSTRUMENTATION_PTR_ == NULL) { if (*) { " << prefix << "HINT_PTR_ = ";
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

			if (this->conf.INSTRUMENT_OBJECTS) {
				// instrumentation: set retired flag
				stream << indent << "(";
				enter.args.at(0)->accept(*this);
				stream << ")->RETIRED_ = true;" << std::endl;

			} else if (!this->conf.INSTRUMENT_FLAG) {
				std::string prefix = this->conf.INSTRUMENT_WRAP_SHARED ? "CC_->" : "";
				if (this->conf.INSTRUMENT_WITH_HINT) {
					stream << indent << "atomic { if (HINT_PROPHECY_) {" << std::endl;
					indent++;
					stream << indent << "HINT_PROPHECY_ = false;" << std::endl;
					stream << indent << "if (" << prefix << "HINT_PTR_ != ";
					enter.args.at(0)->accept(*this);
					stream << ") { fail(); }" << std::endl;
					stream << indent << "if (" << prefix << "HINT_PTR_ == NULL) { fail(); }" << std::endl;
					stream << indent << "if (" << prefix << "INSTRUMENTATION_PTR_ != NULL) { fail(); }" << std::endl;
					stream << indent << "" << prefix << "INSTRUMENTATION_PTR_ = " << prefix << "HINT_PTR_;" << std::endl;
					indent--;
					stream << indent << "}}" << std::endl;
				} else {
					// instrumentation: set if not yet set
					stream << indent << "if (" << prefix << "INSTRUMENTATION_PTR_ == NULL) { if (*) { " << prefix << "INSTRUMENTATION_PTR_ = ";
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
			print_requires(function);
			assert(function.body);
			add_fix_me = true;
			function.body->accept(*this);
			add_fix_me = false;
			stream << std::endl;
		}
	}

	void visit(const Program& program) {
		update_conf(this->conf, program);

		this->program = &program;
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
		if (this->conf.INSTRUMENT_WRAP_SHARED) {
			stream << indent << "class Container_ {" << std::endl;
			indent++;
		}
		for (const auto& decl : program.variables) {
			print_var_def(*decl);
		}
		print_instrumentation_decls();
		if (this->conf.INSTRUMENT_WRAP_SHARED) {
			indent--;
			stream << indent << "}" << std::endl << std::endl;
			stream << indent << "Container_ CC_;" << std::endl;
		}
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

void to_cave_input(const Program& program, const Function& retire_function, std::set<const Assert*>* whitelist, std::ostream& stream) {
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


bool discharge_assertions_impl(const Program& program, const Function& retire_function, std::set<const Assert*>* whitelist) {
	std::string filename = "tmp_memchk.cav";
	std::ofstream outfile(filename);
	to_cave_input(program, retire_function, whitelist, outfile);
	outfile.close();

	// std::string command = "./cave -allow_leaks -lm -por " + filename;
	// TODO: call CAVE executable relativ to working dir
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

bool prtypes::discharge_assertions(const Program& program, const Function& retire_function) {
	return discharge_assertions_impl(program, retire_function, nullptr);
}


bool prtypes::discharge_assertions(const Program& program, const Function& retire_function, const std::vector<std::reference_wrapper<const Assert>>& whitelist) {
	std::set<const Assert*> set;
	for (const Assert& assert : whitelist) {
		set.insert(&assert);
	}
	return discharge_assertions_impl(program, retire_function, &set);
}


bool prtypes::check_linearizability(const cola::Program& program) {
	std::string filename = "tmp_linear.cav";
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

	// TODO: call CAVE executable relativ to working dir
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
