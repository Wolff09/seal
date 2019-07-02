#pragma once
#ifndef COLA_AST
#define COLA_AST


#include <memory>
#include <vector>
#include <map>
#include <string>
#include <cassert>


namespace cola {

	struct AstNode;
	struct VariableDeclaration;
	struct Expression;
	struct BooleanValue;
	struct NullValue;
	struct EmptyValue;
	struct MaxValue;
	struct MinValue;
	struct NDetValue;
	struct VariableExpression;
	struct NegatedExpression;
	struct BinaryExpression;
	struct Dereference;
	struct InvariantExpression;
	struct InvariantActive;
	struct Statement;
	struct AnnotatedStatement;
	struct Sequence;
	struct Scope;
	struct Atomic;
	struct Choice;
	struct IfThenElse;
	struct Loop;
	struct While;
	struct Command;
	struct Skip;
	struct Break;
	struct Continue;
	struct Assume;
	struct Assert;
	struct AngelChoose;
	struct AngelActive;
	struct AngelContains;
	struct Return;
	struct Malloc;
	struct Assignment;
	struct Enter;
	struct Exit;
	struct Macro;
	struct CompareAndSwap;
	struct Function;
	struct Program;


	/*--------------- visitor ---------------*/

	struct Visitor {
		virtual void visit(const VariableDeclaration& node) = 0;
		virtual void visit(const Expression& node) = 0;
		virtual void visit(const BooleanValue& node) = 0;
		virtual void visit(const NullValue& node) = 0;
		virtual void visit(const EmptyValue& node) = 0;
		virtual void visit(const MaxValue& node) = 0;
		virtual void visit(const MinValue& node) = 0;
		virtual void visit(const NDetValue& node) = 0;
		virtual void visit(const VariableExpression& node) = 0;
		virtual void visit(const NegatedExpression& node) = 0;
		virtual void visit(const BinaryExpression& node) = 0;
		virtual void visit(const Dereference& node) = 0;
		virtual void visit(const InvariantExpression& node) = 0;
		virtual void visit(const InvariantActive& node) = 0;
		virtual void visit(const Sequence& node) = 0;
		virtual void visit(const Scope& node) = 0;
		virtual void visit(const Atomic& node) = 0;
		virtual void visit(const Choice& node) = 0;
		virtual void visit(const IfThenElse& node) = 0;
		virtual void visit(const Loop& node) = 0;
		virtual void visit(const While& node) = 0;
		virtual void visit(const Skip& node) = 0;
		virtual void visit(const Break& node) = 0;
		virtual void visit(const Continue& node) = 0;
		virtual void visit(const Assume& node) = 0;
		virtual void visit(const Assert& node) = 0;
		virtual void visit(const AngelChoose& node) = 0;
		virtual void visit(const AngelActive& node) = 0;
		virtual void visit(const AngelContains& node) = 0;
		virtual void visit(const Return& node) = 0;
		virtual void visit(const Malloc& node) = 0;
		virtual void visit(const Assignment& node) = 0;
		virtual void visit(const Enter& node) = 0;
		virtual void visit(const Exit& node) = 0;
		virtual void visit(const Macro& node) = 0;
		virtual void visit(const CompareAndSwap& node) = 0;
		virtual void visit(const Function& node) = 0;
		virtual void visit(const Program& node) = 0;
	};

	struct NonConstVisitor {
		virtual void visit(VariableDeclaration& node) = 0;
		virtual void visit(Expression& node) = 0;
		virtual void visit(BooleanValue& node) = 0;
		virtual void visit(NullValue& node) = 0;
		virtual void visit(EmptyValue& node) = 0;
		virtual void visit(MaxValue& node) = 0;
		virtual void visit(MinValue& node) = 0;
		virtual void visit(NDetValue& node) = 0;
		virtual void visit(VariableExpression& node) = 0;
		virtual void visit(NegatedExpression& node) = 0;
		virtual void visit(BinaryExpression& node) = 0;
		virtual void visit(Dereference& node) = 0;
		virtual void visit(InvariantExpression& node) = 0;
		virtual void visit(InvariantActive& node) = 0;
		virtual void visit(Sequence& node) = 0;
		virtual void visit(Scope& node) = 0;
		virtual void visit(Atomic& node) = 0;
		virtual void visit(Choice& node) = 0;
		virtual void visit(IfThenElse& node) = 0;
		virtual void visit(Loop& node) = 0;
		virtual void visit(While& node) = 0;
		virtual void visit(Skip& node) = 0;
		virtual void visit(Break& node) = 0;
		virtual void visit(Continue& node) = 0;
		virtual void visit(Assume& node) = 0;
		virtual void visit(Assert& node) = 0;
		virtual void visit(AngelChoose& node) = 0;
		virtual void visit(AngelActive& node) = 0;
		virtual void visit(AngelContains& node) = 0;
		virtual void visit(Return& node) = 0;
		virtual void visit(Malloc& node) = 0;
		virtual void visit(Assignment& node) = 0;
		virtual void visit(Enter& node) = 0;
		virtual void visit(Exit& node) = 0;
		virtual void visit(Macro& node) = 0;
		virtual void visit(CompareAndSwap& node) = 0;
		virtual void visit(Function& node) = 0;
		virtual void visit(Program& node) = 0;
	};


	/*--------------- base ---------------*/

	struct AstNode {
		static std::size_t make_id() {
			static std::size_t MAX_ID = 0;
			return MAX_ID++;
		}
		const std::size_t id;
		AstNode() : id(make_id()) {}
		AstNode(const AstNode& other) = delete;
		virtual ~AstNode() = default;
		virtual void accept(NonConstVisitor& visitor) = 0;
		virtual void accept(Visitor& visitor) const = 0;
	};

	#define ACCEPT_COLA_VISITOR \
		virtual void accept(NonConstVisitor& visitor) override { visitor.visit(*this); } \
		virtual void accept(Visitor& visitor) const override { visitor.visit(*this); }


	/*--------------- types ---------------*/

	enum struct Sort {
		VOID, BOOL, DATA, PTR
	};

	struct Type {
		std::string name;
		Sort sort;
		std::map<std::string, std::reference_wrapper<const Type>> fields;
		Type(std::string name_, Sort sort_) : name(name_), sort(sort_) {}
		const Type* field(std::string name) const {
			auto it = fields.find(name);
			if (it == fields.end()) {
				return nullptr;
			} else {
				return &(it->second.get());
			}
		}
		bool has_field(std::string name) const { return field(name) != nullptr; }
		bool operator==(const Type& other) const { return name == other.name; }
		bool operator!=(const Type& other) const { return name != other.name; }
		static const Type& void_type() { static const Type type = Type("void", Sort::VOID); return type; }
		static const Type& bool_type() { static const Type type = Type("bool", Sort::BOOL); return type; }
		static const Type& data_type() { static const Type type = Type("data_t", Sort::DATA); return type; }
		static const Type& null_type() { static const Type type = Type("nullptr", Sort::PTR); return type; }
	};

	inline bool assignable(const Type& to, const Type& from) { // TODO: remove static
		if (to.sort != Sort::PTR) {
			return to.sort == from.sort;
		} else {
			return (to != Type::null_type())
			    && (to == from || from == Type::null_type());
		}
	}

	inline bool comparable(const Type& type, const Type& other) {
		if (type == other) return true;
		else if (type.sort == Sort::PTR && other == Type::null_type()) return true;
		else if (other.sort == Sort::PTR && type == Type::null_type()) return true;
		else return false;
	}

	struct VariableDeclaration : public AstNode {
		std::string name;
		const Type& type;
		bool is_shared = false;
		VariableDeclaration(std::string name_, const Type& type_, bool shared_) : name(name_), type(type_), is_shared(shared_) {}
		VariableDeclaration(const VariableDeclaration&) = delete; // cautiously delete copy construction to prevent bad things from happening
		ACCEPT_COLA_VISITOR
	};


	/*--------------- expressions ---------------*/

	struct Expression : virtual public AstNode {
		virtual ~Expression() = default;
		virtual const Type& type() const = 0;
		Sort sort() const { return type().sort; }
	};

	struct BooleanValue : public Expression {
		bool value;
		BooleanValue(bool value_) : value(value_) {};
		const Type& type() const override { return Type::bool_type(); }
		ACCEPT_COLA_VISITOR
	};

	struct NullValue : public Expression {
		const Type& type() const override { return Type::null_type(); }
		ACCEPT_COLA_VISITOR
	};

	struct EmptyValue : public Expression {
		const Type& type() const override { return Type::data_type(); }
		ACCEPT_COLA_VISITOR
	};

	struct MaxValue : public Expression {
		const Type& type() const override { return Type::data_type(); }
		ACCEPT_COLA_VISITOR
	};

	struct MinValue : public Expression {
		const Type& type() const override { return Type::data_type(); }
		ACCEPT_COLA_VISITOR
	};

	struct NDetValue : public Expression {
		const Type& type() const override { return Type::data_type(); }
		ACCEPT_COLA_VISITOR
	};

	struct VariableExpression : public Expression {
		const VariableDeclaration& decl;
		VariableExpression(const VariableDeclaration& decl_) : decl(decl_) {}
		const Type& type() const override { return decl.type; }
		ACCEPT_COLA_VISITOR
	};

	struct NegatedExpression : public Expression {
		std::unique_ptr<Expression> expr;
		NegatedExpression(std::unique_ptr<Expression> expr_) : expr(std::move(expr_)) {
			assert(expr);
			assert(expr->sort() == Sort::BOOL);
		}
		const Type& type() const override { return Type::bool_type(); }
		ACCEPT_COLA_VISITOR
	};

	struct BinaryExpression : public Expression {
		enum struct Operator {
			EQ, NEQ, LEQ, LT, GEQ, GT, AND, OR
		};
		Operator op;
		std::unique_ptr<Expression> lhs;
		std::unique_ptr<Expression> rhs;
		BinaryExpression(Operator op_, std::unique_ptr<Expression> lhs_, std::unique_ptr<Expression> rhs_) : op(op_), lhs(std::move(lhs_)), rhs(std::move(rhs_)) {
			assert(lhs);
			assert(rhs);
			assert(lhs->sort() == rhs->sort());
		}
		const Type& type() const override { return Type::bool_type(); }
		ACCEPT_COLA_VISITOR
	};

	inline std::string toString(BinaryExpression::Operator op) {
		switch (op) {
			case BinaryExpression::Operator::EQ: return "==";
			case BinaryExpression::Operator::NEQ: return "!=";
			case BinaryExpression::Operator::LEQ: return "<=";
			case BinaryExpression::Operator::LT: return "<";
			case BinaryExpression::Operator::GEQ: return ">=";
			case BinaryExpression::Operator::GT: return ">";
			case BinaryExpression::Operator::AND: return "&&";
			case BinaryExpression::Operator::OR: return "||";
		}
	}

	struct Dereference : public Expression {
		std::unique_ptr<Expression> expr;
		std::string fieldname;
		Dereference(std::unique_ptr<Expression> expr_, std::string fieldname_) : expr(std::move(expr_)), fieldname(fieldname_) {
			assert(expr);
			assert(expr->type().field(fieldname) != nullptr);
		}
		const Type& type() const override {
			const Type* type = expr->type().field(fieldname);
			assert(type);
			return *type;
		}
		ACCEPT_COLA_VISITOR
	};

	struct Invariant : public AstNode {
		std::unique_ptr<Expression> expr;
		Invariant(std::unique_ptr<Expression> expr_) : expr(std::move(expr_)) {
			assert(expr);
		}
	};

	struct InvariantExpression : public Invariant {
		InvariantExpression(std::unique_ptr<Expression> expr_) : Invariant(std::move(expr_)) {}
		ACCEPT_COLA_VISITOR
	};

	struct InvariantActive : public Invariant {
		InvariantActive(std::unique_ptr<Expression> expr_) : Invariant(std::move(expr_)) {}
		ACCEPT_COLA_VISITOR
	};


	/*--------------- statements ---------------*/

	struct Statement : virtual public AstNode {
	};

	struct AnnotatedStatement : public Statement {
		std::unique_ptr<Invariant> annotation; // optional
	};

	struct Sequence : public Statement {
		std::unique_ptr<Statement> first;
		std::unique_ptr<Statement> second;
		Sequence(std::unique_ptr<Statement> first_, std::unique_ptr<Statement> second_) : first(std::move(first_)), second(std::move(second_)) {
			assert(first);
			assert(second);
		}
		ACCEPT_COLA_VISITOR
	};

	struct Scope : public Statement {
		std::vector<std::unique_ptr<VariableDeclaration>> variables;
		std::unique_ptr<Statement> body;
		Scope(std::unique_ptr<Statement> body_) : body(std::move(body_)) {
			assert(body);
		}
		ACCEPT_COLA_VISITOR
	};

	struct Atomic : public AnnotatedStatement {
		std::unique_ptr<Scope> body;
		Atomic(std::unique_ptr<Scope> body_) : body(std::move(body_)) {
			assert(body);
		}
		ACCEPT_COLA_VISITOR
	};

	struct Choice : public Statement {
		std::vector<std::unique_ptr<Scope>> branches;
		ACCEPT_COLA_VISITOR
	};

	struct IfThenElse : public AnnotatedStatement {
		std::unique_ptr<Expression> expr;
		std::unique_ptr<Scope> ifBranch;
		std::unique_ptr<Scope> elseBranch;
		IfThenElse(std::unique_ptr<Expression> expr_, std::unique_ptr<Scope> ifBranch_, std::unique_ptr<Scope> elseBranch_) : expr(std::move(expr_)), ifBranch(std::move(ifBranch_)), elseBranch(std::move(elseBranch_)) {
			assert(expr);
			assert(ifBranch);
			assert(elseBranch);
		}
		ACCEPT_COLA_VISITOR
	};

	struct Loop : public Statement {
		std::unique_ptr<Scope> body;
		Loop(std::unique_ptr<Scope> body_) : body(std::move(body_)) {
			assert(body);
		}
		ACCEPT_COLA_VISITOR
	};

	struct While : public AnnotatedStatement {
		std::unique_ptr<Expression> expr;
		std::unique_ptr<Scope> body;
		While(std::unique_ptr<Expression> expr_, std::unique_ptr<Scope> body_) : expr(std::move(expr_)), body(std::move(body_)) {
			assert(expr);
			assert(body);
		}
		ACCEPT_COLA_VISITOR
	};


	/*--------------- commands ---------------*/

	struct Command : public AnnotatedStatement {
	};

	struct Skip : public Command {
		ACCEPT_COLA_VISITOR
	};

	struct Break : public Command {
		ACCEPT_COLA_VISITOR
	};

	struct Continue : public Command {
		ACCEPT_COLA_VISITOR
	};

	struct Assume : public Command {
		std::unique_ptr<Expression> expr;
		Assume(std::unique_ptr<Expression> expr_) : expr(std::move(expr_)) {
			assert(expr);
		}
		ACCEPT_COLA_VISITOR
	};

	struct Assert : public Command {
		std::unique_ptr<Invariant> inv;
		Assert(std::unique_ptr<Invariant> inv_) : inv(std::move(inv_)) {
			assert(inv);
		}
		ACCEPT_COLA_VISITOR
	};

	struct AngelChoose : public Command {
		ACCEPT_COLA_VISITOR
	};

	struct AngelActive : public Command {
		ACCEPT_COLA_VISITOR
	};

	struct AngelContains : public Command {
		const VariableDeclaration& var;
		AngelContains(const VariableDeclaration& var_) : var(var_) {}
		ACCEPT_COLA_VISITOR
	};

	struct Return : public Command {
		std::unique_ptr<Expression> expr; // optional
		Return() {};
		Return(std::unique_ptr<Expression> expr_) : expr(std::move(expr_)) {
			assert(expr);
		}
		ACCEPT_COLA_VISITOR
	};

	struct Malloc : public Command {
		const VariableDeclaration& lhs;
		Malloc(const VariableDeclaration& lhs_) : lhs(lhs_) {
			assert(lhs.type.sort == Sort::PTR);
		}
		ACCEPT_COLA_VISITOR
	};

	struct Assignment : public Command {
		std::unique_ptr<Expression> lhs;
		std::unique_ptr<Expression> rhs;
		Assignment(std::unique_ptr<Expression> lhs_, std::unique_ptr<Expression> rhs_) : lhs(std::move(lhs_)), rhs(std::move(rhs_)) {
			assert(lhs);
			assert(rhs);
			assert(assignable(lhs->type(), rhs->type()));
		}
		ACCEPT_COLA_VISITOR
	};

	struct Enter : public Command {
		const Function& decl;
		std::vector<std::unique_ptr<Expression>> args;
		Enter(const Function& decl_) : decl(decl_) {
			// TODO: assert(decl.kind == Function::SMR);
		}
		ACCEPT_COLA_VISITOR
	};

	struct Exit : public Command {
		const Function& decl;
		Exit(const Function& decl_) : decl(decl_) {
			// TODO: assert(decl.kind == Function::SMR);
		}
		ACCEPT_COLA_VISITOR
	};

	struct Macro : public Command {
		const Function& decl;
		std::vector<std::unique_ptr<Expression>> args;
		Macro(const Function& decl_) : decl(decl_) {
			// TODO: assert(decl.kind == Function::MACRO);
		}
		ACCEPT_COLA_VISITOR
	};


	/*--------------- CAS ---------------*/

	struct CompareAndSwap : public Command, public Expression {
		struct Triple {
			std::unique_ptr<Expression> dst;
			std::unique_ptr<Expression> cmp;
			std::unique_ptr<Expression> src;
			Triple(std::unique_ptr<Expression> dst_, std::unique_ptr<Expression> cmp_, std::unique_ptr<Expression> src_) : dst(std::move(dst_)), cmp(std::move(cmp_)), src(std::move(src_)) {
				assert(dst);
				assert(cmp);
				assert(src);
			}
		};
		std::vector<Triple> elems;
		const Type& type() const override { return Type::bool_type(); }
		ACCEPT_COLA_VISITOR
	};


	/*--------------- functions/summaries/programs ---------------*/

	struct Function : public AstNode {
		// invariant: body != nullptr iff kind != SMR
		enum Kind { INTERFACE, SMR, MACRO };
		std::string name;
		const Type& return_type;
		Kind kind;
		std::vector<std::unique_ptr<VariableDeclaration>> args;
		std::unique_ptr<Scope> body;
		Function(std::string name_, const Type& returnType_, Kind kind_) : name(name_), return_type(returnType_), kind(kind_) {}
		ACCEPT_COLA_VISITOR
	};

	struct Program : public AstNode {
		std::string name;
		std::vector<std::unique_ptr<Type>> types;
		std::vector<std::unique_ptr<VariableDeclaration>> variables;
		std::unique_ptr<Function> initializer;
		std::vector<std::unique_ptr<Function>> functions;
		std::map<std::string, std::string> options;
		ACCEPT_COLA_VISITOR
	};


	/*--------------- base visitor ---------------*/

	// struct BaseVisitor : public Visitor, public NonConstVisitor {
	// 	virtual void visit(const VariableDeclaration& /*node*/) {}
	// 	virtual void visit(const Expression& /*node*/) {}
	// 	virtual void visit(const BooleanValue& /*node*/) {}
	// 	virtual void visit(const NullValue& /*node*/) {}
	// 	virtual void visit(const EmptyValue& /*node*/) {}
	// 	virtual void visit(const NDetValue& /*node*/) {}
	// 	virtual void visit(const VariableExpression& /*node*/) {}
	// 	virtual void visit(const NegatedExpression& /*node*/) {}
	// 	virtual void visit(const BinaryExpression& /*node*/) {}
	// 	virtual void visit(const Dereference& /*node*/) {}
	// 	virtual void visit(const InvariantExpression& /*node*/) {}
	// 	virtual void visit(const InvariantActive& /*node*/) {}
	// 	virtual void visit(const Sequence& /*node*/) {}
	// 	virtual void visit(const Scope& /*node*/) {}
	// 	virtual void visit(const Atomic& /*node*/) {}
	// 	virtual void visit(const Choice& /*node*/) {}
	// 	virtual void visit(const IfThenElse& /*node*/) {}
	// 	virtual void visit(const Loop& /*node*/) {}
	// 	virtual void visit(const While& /*node*/) {}
	// 	virtual void visit(const Skip& /*node*/) {}
	// 	virtual void visit(const Break& /*node*/) {}
	// 	virtual void visit(const Continue& /*node*/) {}
	// 	virtual void visit(const Assume& /*node*/) {}
	// 	virtual void visit(const Assert& /*node*/) {}
	// 	virtual void visit(const Return& /*node*/) {}
	// 	virtual void visit(const Malloc& /*node*/) {}
	// 	virtual void visit(const Assignment& /*node*/) {}
	// 	virtual void visit(const Enter& /*node*/) {}
	// 	virtual void visit(const Exit& /*node*/) {}
	// 	virtual void visit(const Macro& /*node*/) {}
	// 	virtual void visit(const CompareAndSwap& /*node*/) {}
	// 	virtual void visit(const Function& /*node*/) {}
	// 	virtual void visit(const Program& /*node*/) {}
	// 	virtual void visit(VariableDeclaration& /*node*/) {}
	// 	virtual void visit(Expression& /*node*/) {}
	// 	virtual void visit(BooleanValue& /*node*/) {}
	// 	virtual void visit(NullValue& /*node*/) {}
	// 	virtual void visit(EmptyValue& /*node*/) {}
	// 	virtual void visit(NDetValue& /*node*/) {}
	// 	virtual void visit(VariableExpression& /*node*/) {}
	// 	virtual void visit(NegatedExpression& /*node*/) {}
	// 	virtual void visit(BinaryExpression& /*node*/) {}
	// 	virtual void visit(Dereference& /*node*/) {}
	// 	virtual void visit(InvariantExpression& /*node*/) {}
	// 	virtual void visit(InvariantActive& /*node*/) {}
	// 	virtual void visit(Sequence& /*node*/) {}
	// 	virtual void visit(Scope& /*node*/) {}
	// 	virtual void visit(Atomic& /*node*/) {}
	// 	virtual void visit(Choice& /*node*/) {}
	// 	virtual void visit(IfThenElse& /*node*/) {}
	// 	virtual void visit(Loop& /*node*/) {}
	// 	virtual void visit(While& /*node*/) {}
	// 	virtual void visit(Skip& /*node*/) {}
	// 	virtual void visit(Break& /*node*/) {}
	// 	virtual void visit(Continue& /*node*/) {}
	// 	virtual void visit(Assume& /*node*/) {}
	// 	virtual void visit(Assert& /*node*/) {}
	// 	virtual void visit(Return& /*node*/) {}
	// 	virtual void visit(Malloc& /*node*/) {}
	// 	virtual void visit(Assignment& /*node*/) {}
	// 	virtual void visit(Enter& /*node*/) {}
	// 	virtual void visit(Exit& /*node*/) {}
	// 	virtual void visit(Macro& /*node*/) {}
	// 	virtual void visit(CompareAndSwap& /*node*/) {}
	// 	virtual void visit(Function& /*node*/) {}
	// 	virtual void visit(Program& /*node*/) {}
	// };

} // namespace cola

#endif
