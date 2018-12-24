#pragma once // TODO: add include guards

#include <memory>
#include <vector>
#include <map>
#include <string>


struct AstNode;
struct Function;
struct Program;


struct Visitor {
	virtual void accept(AstNode& node) = 0;
	virtual void accept(const AstNode& node) = 0;
};

struct AstNode {
	virtual ~AstNode() = default;
	virtual void accept_visitor(Visitor& visitor) = 0;
	virtual void accept_visitor(Visitor& visitor) const = 0;
};

#define ACCEPT_VISITOR \
        virtual void accept_visitor(Visitor& visitor) override { visitor.accept(*this); } \
        virtual void accept_visitor(Visitor& visitor) const override { visitor.accept(*this); }


/*--------------- types ---------------*/

enum struct Sort {
	VOID, BOOL, DATA, PTR
};

struct Type {
	std::string name;
	Sort sort;
	std::map<std::string, std::reference_wrapper<const Type>> fields;
	Type(std::string name, Sort sort) : name(name), sort(sort) {}
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

static bool assignable(const Type& to, const Type& from) { // TODO: remove static
	if (to.sort != Sort::PTR) {
		return to.sort == from.sort;
	} else {
		return (to != Type::null_type())
		    && (to == from || from == Type::null_type());
	}
}

static bool comparable(const Type& type, const Type& other) {
	if (type == other) return true;
	else if (type.sort == Sort::PTR && other == Type::null_type()) return true;
	else if (other.sort == Sort::PTR && type == Type::null_type()) return true;
	else return false;
}

struct VariableDeclaration : public AstNode {
	std::string name;
	const Type& type;
	bool is_shared = false;
	VariableDeclaration(std::string name, const Type& type, bool shared) : name(name), type(type), is_shared(shared) {}
	ACCEPT_VISITOR
};


/*--------------- expressions ---------------*/

struct Expression : public AstNode {
	virtual ~Expression() = default;
	virtual const Type& type() const;
	Sort sort() const { return type().sort; }
};

struct BooleanValue : public Expression {
	bool value;
	BooleanValue(bool value) : value(value) {};
	const Type& type() const override { return Type::bool_type(); }
	ACCEPT_VISITOR
};

struct NullValue : public Expression {
	const Type& type() const override { return Type::null_type(); }
	ACCEPT_VISITOR
};

struct EmptyValue : public Expression {
	const Type& type() const override { return Type::data_type(); }
	ACCEPT_VISITOR
};

struct NDetValue : public Expression {
	const Type& type() const override { return Type::data_type(); }
	ACCEPT_VISITOR
};

struct VariableExpression : public Expression {
	const VariableDeclaration& decl;
	VariableExpression(const VariableDeclaration& decl) : decl(decl) {}
	const Type& type() const override { return decl.type; }
	ACCEPT_VISITOR
};

struct NegatedExpresion : public Expression {
	std::unique_ptr<Expression> expr;
	NegatedExpresion(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {
		assert(expr);
		assert(expr->sort() == Sort::BOOL);
	}
	const Type& type() const override { return Type::bool_type(); }
	ACCEPT_VISITOR
};

struct BinaryExpression : public Expression {
	enum struct Operator {
		EQ, NEQ, LEQ, LT, GEQ, GT, AND, OR
	};
	Operator op;
	std::unique_ptr<Expression> lhs;
	std::unique_ptr<Expression> rhs;
	BinaryExpression(Operator op, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs) : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {
		assert(lhs);
		assert(rhs);
		assert(lhs->sort() == Sort::BOOL);
		assert(rhs->sort() == Sort::BOOL);
	}
	const Type& type() const override { return Type::bool_type(); }
	ACCEPT_VISITOR
};

static std::string toString(BinaryExpression::Operator op) {
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
	Dereference(std::unique_ptr<Expression> expr, std::string fieldname) : expr(std::move(expr)), fieldname(fieldname) {
		assert(expr);
		assert(expr->type().field(fieldname) != nullptr);
	}
	const Type& type() const override {
		const Type* type = expr->type().field(fieldname);
		assert(type);
		return *type;
	}
	ACCEPT_VISITOR
};


/*--------------- statements ---------------*/

struct Statement : public AstNode {
};

struct Sequence : public Statement {
	std::unique_ptr<Statement> first;
	std::unique_ptr<Statement> second;
	Sequence(std::unique_ptr<Statement> first, std::unique_ptr<Statement> second) : first(std::move(first)), second(std::move(second)) {
		assert(first);
		assert(second);
	}
	ACCEPT_VISITOR
};

struct Atomic : public Statement {
	std::unique_ptr<Statement> stmt;
	Atomic(std::unique_ptr<Statement> stmt) : stmt(std::move(stmt)) {
		assert(stmt);
	}
	ACCEPT_VISITOR
};

struct Choice : public Statement {
	std::vector<std::unique_ptr<Statement>> branches;
	ACCEPT_VISITOR
};

struct Loop : public Statement {
	std::unique_ptr<Statement> body;
	ACCEPT_VISITOR
};


/*--------------- commands ---------------*/

struct Command : public Statement {
};

struct Skip : public Command {
	ACCEPT_VISITOR
};

struct Break : public Command {
	ACCEPT_VISITOR
};

struct Continue : public Command {
	ACCEPT_VISITOR
};

struct Assume : public Command {
	std::unique_ptr<Expression> expr;
	Assume(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {
		assert(expr);
	}
	ACCEPT_VISITOR
};

struct Assert : public Command {
	std::unique_ptr<Expression> expr;
	Assert(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {
		assert(expr);
	}
	ACCEPT_VISITOR
};

struct Return : public Command {
	std::unique_ptr<Expression> expr;
	Return() {};
	Return(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {
		assert(expr);
	}
};

struct Malloc : public Command {
	std::unique_ptr<Expression> lhs;
	Malloc(std::unique_ptr<Expression> lhs) : lhs(std::move(lhs)) {
		assert(lhs);
	}
	ACCEPT_VISITOR
};

struct Assignment : public Command {
	std::unique_ptr<Expression> lhs;
	std::unique_ptr<Expression> rhs;
	Assignment(std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) {
		assert(lhs);
		assert(rhs);
		assert(assignable(lhs->type(), rhs->type()));
	}
	ACCEPT_VISITOR
};

struct Enter : public Command {
	const Function& decl;
	std::vector<std::unique_ptr<Expression>> args;
	Enter(const Function& decl) : decl(decl) {
		// TODO: assert(decl.kind == Function::SMR);
	}
	ACCEPT_VISITOR
};

struct Exit : public Command {
	const Function& decl;
	Exit(const Function& decl) : decl(decl) {
		// TODO: assert(decl.kind == Function::SMR);
	}
	ACCEPT_VISITOR
};

struct Macro : public Command {
	const Function& decl;
	Macro(const Function& decl) : decl(decl) {
		// TODO: assert(decl.kind == Function::MACRO);
	}
	ACCEPT_VISITOR
};


/*--------------- functions/summaries/programs ---------------*/

struct Function : public AstNode {
	// invariant: body != nullptr iff kind != SMR
	enum Kind { INTERFACE, SMR, MACRO };
	std::string name;
	const Type& return_type;
	Kind kind;
	std::vector<std::unique_ptr<VariableDeclaration>> variables;
	std::unique_ptr<Statement> body;
	Function(std::string name, const Type& returnType, Kind kind) : name(name), return_type(returnType), kind(kind) {}
	ACCEPT_VISITOR
};

struct Program : public AstNode {
	std::string name;
	std::vector<std::unique_ptr<Type>> types;
	std::vector<std::unique_ptr<VariableDeclaration>> variables;
	std::unique_ptr<Function> initalizer;
	std::vector<std::unique_ptr<Function>> functions;
	ACCEPT_VISITOR
};
