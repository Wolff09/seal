#include "cola/parser/AstBuilder.hpp"

#include "cola/parser/TypeBuilder.hpp"
#include "cola/util.hpp"
#include <sstream>

using namespace cola;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AstBuilder::pushScope() {
	_scope.push_back(VariableMap());
}

std::vector<std::unique_ptr<VariableDeclaration>> AstBuilder::popScope() {
	std::vector<std::unique_ptr<VariableDeclaration>> decls, tmp;
	decls.reserve(_scope.back().size());
	tmp.reserve(_scope.back().size());
	for (auto& entry : _scope.back()) { // reverses variable order?
		tmp.push_back(std::move(entry.second));
	}
	while (!tmp.empty()) {
		decls.push_back(std::move(tmp.back()));
		tmp.pop_back();
	}
	_scope.pop_back();
	return decls;
}

bool AstBuilder::isVariableDeclared(std::string variableName) {
	for (auto it = _scope.crbegin(); it != _scope.rend(); it++) {
		if (it->count(variableName)) {
			return true;
		}
	}

	return false;
}

const VariableDeclaration& AstBuilder::lookupVariable(std::string variableName) {
	std::size_t counter = 0;
	for (auto it = _scope.crbegin(); it != _scope.rend(); it++) {
		if (it->count(variableName)) {
			return *it->at(variableName);
		}
		counter++;
	}

	throw std::logic_error("Variable '" + variableName + "' not declared.");
}

void AstBuilder::addVariable(std::unique_ptr<VariableDeclaration> variable) {
	if (isVariableDeclared(variable->name)) {
		throw std::logic_error("CompilationError: variable '" + variable->name + "' already declared.");
	}

	_scope.back()[variable->name] = std::move(variable);
}

bool AstBuilder::isTypeDeclared(std::string typeName) {
	return _types.count(typeName) != 0;
}

const Type& AstBuilder::lookupType(std::string typeName) {
	auto it = _types.find(typeName);
	if (it == _types.end()) {
		std::string help = _types.count(typeName + "*") ? " Did you mean '" + typeName + "*'?" : "";
		throw std::logic_error("Type '" + typeName + "' not declared." + help);
	} else {
		return it->second;
	}
}

std::unique_ptr<Statement> AstBuilder::mk_stmt_from_list(std::vector<cola::CoLaParser::StatementContext*> stmts) {
	std::vector<Statement*> list;
	stmts.reserve(stmts.size());
	for (auto& stmt : stmts) {
		list.push_back(stmt->accept(this).as<Statement*>());
	}
	return mk_stmt_from_list(list);
}

std::unique_ptr<Statement> AstBuilder::mk_stmt_from_list(std::vector<Statement*> stmts) {
	if (stmts.size() == 0) {
		return std::make_unique<Skip>();
	} else {
		std::unique_ptr<Statement> current(stmts.at(0));
		for (std::size_t i = 1; i < stmts.size(); i++) {
			std::unique_ptr<Statement> next(stmts.at(i));
			auto seq = std::make_unique<Sequence>(std::move(current), std::move(next));
			current = std::move(seq);
		}
		return current;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Program> AstBuilder::buildFrom(cola::CoLaParser::ProgramContext* parseTree) {
	AstBuilder builder;
	parseTree->accept(&builder);
	return builder._program;
}

antlrcpp::Any AstBuilder::visitProgram(cola::CoLaParser::ProgramContext* context) {
	_program = std::make_unique<Program>();
	_program->name = "unknown";

	// get options
	for (auto optionContext : context->opts()) {
		optionContext->accept(this);
	}

	// get types
	auto typeinfo = TypeBuilder::build(context);
	_types = std::move(typeinfo->lookup);
	_program->types = std::move(typeinfo->types);

	// add shared variables
	pushScope();
	for (auto variableContext : context->var_decl()) {
		variableContext->accept(this);
	}
	for (auto& kvpair : _scope.back()) {
		kvpair.second->is_shared = true;
	}

	// functions/summaries
	_program->functions.reserve(context->function().size());
	for (auto functionContext : context->function()) {
		functionContext->accept(this);
	}

	if (!_program->initializer) {
		throw std::logic_error("Program has no 'init' function.");
	}

	// finish
	_program->variables = popScope();

	return nullptr;
}

antlrcpp::Any AstBuilder::visitFunction(cola::CoLaParser::FunctionContext* context) {
	std::string name = context->name->getText();
	const Type& returnType = lookupType(context->returnType->accept(this).as<std::string>());
	
	Function::Kind kind = Function::INTERFACE;
	assert(!(context->Inline() && context->Extern()));
	if (context->Inline()) {
		kind = Function::MACRO;
	} else if (context->Extern()) {
		kind = Function::SMR;
	}

	if (kind == Function::SMR && returnType != Type::void_type()) {
		throw std::logic_error("Return type of extern SMR function " + name + " not support: must be of type 'void'.");
	}
	if (kind == Function::MACRO && returnType != Type::void_type()) {
		throw std::logic_error("Return type of inline function " + name + " not support: must be of type 'void'.");
	}

	if (name == INIT_NAME) {
		if (kind != Function::INTERFACE) {
			throw std::logic_error("initializer function '" + name + "' must not have modifier.");
		}
		if (returnType != Type::void_type()) {
			throw std::logic_error("initializer function '" + name + "' must have 'void' return type.");
		}
	}
	if (name == "active") {
		throw std::logic_error("Name clash: function name must not be 'active'.");
	}

	auto function = std::make_unique<Function>(name, returnType, kind);
	_currentFunction = function.get();
	if (name != INIT_NAME) {
		// make function available for calls
		if (_functions.count(name)) {
			throw std::logic_error("Duplicate function declaration: function with name '" + name + "' already defined.");
		}
		_functions.insert({{ name, *function }});
	}

	// add arguments
	pushScope();
	auto arglist = context->args->accept(this).as<ArgDeclList*>();
	function->args.reserve(arglist->size());
	for (auto& pair : *arglist) {
		std::string argname = pair.first;
		const Type& argtype = _types.at(pair.second);
		auto decl = std::make_unique<VariableDeclaration>(argname, argtype, false);
		addVariable(std::move(decl));

		if (argtype.sort == Sort::VOID) {
			throw std::logic_error("Argument type 'void' not supported for argument '" + argname + "'.");
		}
		if (kind == Function::INTERFACE && argtype.sort == Sort::PTR) {
			throw std::logic_error("Argument with name '" + argname + "' of pointer sort not supported for interface functions.");
		}
	}

	// handle body
	if (context->body) {
		assert(kind != Function::SMR);
		auto body = context->body->accept(this).as<Scope*>();
		function->body = std::unique_ptr<Scope>(body);
	} else {
		assert(kind == Function::SMR);
	}

	// get variable decls
	function->args = popScope();

	// transfer ownershp
	if (name == INIT_NAME) {
		_program->initializer = std::move(function);
	} else {
		_program->functions.push_back(std::move(function));
	}

	// TODO: check if every path returns

	return nullptr;
}

antlrcpp::Any AstBuilder::visitArgDeclList(cola::CoLaParser::ArgDeclListContext* context) {
	auto dlist = new ArgDeclList(); //std::make_shared<ArgDeclList>();
	auto size = context->argTypes.size();
	dlist->reserve(size);
	assert(size == context->argNames.size());
	for (std::size_t i = 0; i < size; i++) {
		std::string name = context->argNames.at(i)->getText();
		const Type& type = lookupType(context->argTypes.at(i)->accept(this).as<std::string>());
		// TODO: what the ****?! why can't I pass the reference wrapper here without them being lost after the loop?
		dlist->push_back(std::make_pair(name, type.name));
	}
	return dlist;
}

antlrcpp::Any AstBuilder::visitScope(cola::CoLaParser::ScopeContext* context) {
	pushScope();
	// handle variables
	auto vars = context->var_decl();
	for (auto varDeclContext : vars) {
		varDeclContext->accept(this);
	}

	// handle body
	auto body = mk_stmt_from_list(context->statement());
	assert(body);

	auto scope = new Scope(std::move(body));
	scope->variables = popScope();
	return scope;
}

antlrcpp::Any AstBuilder::visitStruct_decl(cola::CoLaParser::Struct_declContext* /*context*/) {
	throw std::logic_error("Unsupported operation."); // this is done in TypeBuilder.hpp
}

antlrcpp::Any AstBuilder::visitField_decl(cola::CoLaParser::Field_declContext* /*context*/) {
	throw std::logic_error("Unsupported operation."); // this is done in TypeBuilder.hpp
}

antlrcpp::Any AstBuilder::visitNameVoid(cola::CoLaParser::NameVoidContext* /*context*/) {
	std::string name = "void";
	return name;
}

antlrcpp::Any AstBuilder::visitNameBool(cola::CoLaParser::NameBoolContext* /*context*/) {
	std::string name = "bool";
	return name;
}

antlrcpp::Any AstBuilder::visitNameInt(cola::CoLaParser::NameIntContext* /*context*/) {
	throw std::logic_error("Type 'int' not supported. Use 'data_t' instead.");
}

antlrcpp::Any AstBuilder::visitNameData(cola::CoLaParser::NameDataContext* /*context*/) {
	std::string name = "data_t";
	return name;
}

antlrcpp::Any AstBuilder::visitNameIdentifier(cola::CoLaParser::NameIdentifierContext* context) {
	std::string name = context->Identifier()->getText();
	return name;
}

antlrcpp::Any AstBuilder::visitTypeValue(cola::CoLaParser::TypeValueContext* context) {
	std::string name = context->name->accept(this).as<std::string>();
	return name;
}

antlrcpp::Any AstBuilder::visitTypePointer(cola::CoLaParser::TypePointerContext* context) {
	std::string name = context->name->accept(this).as<std::string>() + "*";
	return name;
}

antlrcpp::Any AstBuilder::visitVar_decl(cola::CoLaParser::Var_declContext* context) {
	const Type& type = lookupType(context->type()->accept(this).as<std::string>());
	for (auto token : context->names) {
		std::string name = token->getText();
		auto decl = std::make_unique<VariableDeclaration>(name, type, false); // default to non-shared
		addVariable(std::move(decl));
	}
	return nullptr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

antlrcpp::Any AstBuilder::visitOpEq(cola::CoLaParser::OpEqContext* /*context*/) {
	return BinaryExpression::Operator::EQ;
}

antlrcpp::Any AstBuilder::visitOpNeq(cola::CoLaParser::OpNeqContext* /*context*/) {
	return BinaryExpression::Operator::NEQ;
}

antlrcpp::Any AstBuilder::visitOpLt(cola::CoLaParser::OpLtContext* /*context*/) {
	return BinaryExpression::Operator::LT;
}

antlrcpp::Any AstBuilder::visitOpLte(cola::CoLaParser::OpLteContext* /*context*/) {
	return BinaryExpression::Operator::LEQ;
}

antlrcpp::Any AstBuilder::visitOpGt(cola::CoLaParser::OpGtContext* /*context*/) {
	return BinaryExpression::Operator::GT;
}

antlrcpp::Any AstBuilder::visitOpGte(cola::CoLaParser::OpGteContext* /*context*/) {
	return BinaryExpression::Operator::GEQ;
}

antlrcpp::Any AstBuilder::visitOpAnd(cola::CoLaParser::OpAndContext* /*context*/) {
	return BinaryExpression::Operator::AND;
}

antlrcpp::Any AstBuilder::visitOpOr(cola::CoLaParser::OpOrContext* /*context*/) {
	return BinaryExpression::Operator::OR;
}

static Expression* as_expression(Expression* expr) {
	return expr;
}

antlrcpp::Any AstBuilder::visitValueNull(cola::CoLaParser::ValueNullContext* /*context*/) {
	return as_expression(new NullValue());
}

antlrcpp::Any AstBuilder::visitValueTrue(cola::CoLaParser::ValueTrueContext* /*context*/) {
	return as_expression(new BooleanValue(true));
}

antlrcpp::Any AstBuilder::visitValueFalse(cola::CoLaParser::ValueFalseContext* /*context*/) {
	return as_expression(new BooleanValue(false));
}

antlrcpp::Any AstBuilder::visitValueNDet(cola::CoLaParser::ValueNDetContext* /*context*/) {
	return as_expression(new NDetValue());
}

antlrcpp::Any AstBuilder::visitValueEmpty(cola::CoLaParser::ValueEmptyContext* /*context*/) {
	return as_expression(new EmptyValue());
}

antlrcpp::Any AstBuilder::visitValueMax(cola::CoLaParser::ValueMaxContext* /*context*/) {
	return as_expression(new MaxValue());
}

antlrcpp::Any AstBuilder::visitValueMin(cola::CoLaParser::ValueMinContext* /*context*/) {
	return as_expression(new MinValue());
}

antlrcpp::Any AstBuilder::visitExprValue(cola::CoLaParser::ExprValueContext* context) {
	return as_expression(context->value()->accept(this));
}

antlrcpp::Any AstBuilder::visitExprParens(cola::CoLaParser::ExprParensContext* context) {
	return as_expression(context->expr->accept(this));
}

antlrcpp::Any AstBuilder::visitExprIdentifier(cola::CoLaParser::ExprIdentifierContext* context) {
	std::string name = context->name->getText();
	const VariableDeclaration& decl = lookupVariable(name);
	return as_expression(new VariableExpression(decl));
}

antlrcpp::Any AstBuilder::visitExprNegation(cola::CoLaParser::ExprNegationContext* context) {
	auto expr = context->expr->accept(this).as<Expression*>();
	return as_expression(new NegatedExpression(std::unique_ptr<Expression>(expr)));
}

antlrcpp::Any AstBuilder::visitExprDeref(cola::CoLaParser::ExprDerefContext* context) {
	auto expr = context->expr->accept(this).as<Expression*>();
	std::string fieldname = context->field->getText();
	const Type& exprtype = expr->type();

	if (exprtype.sort != Sort::PTR) {
		throw std::logic_error("Cannot dereference expression of non-pointer type.");
	}
	if (exprtype == Type::null_type()) {
		throw std::logic_error("Cannot dereference 'null'.");
	}
	if (!exprtype.has_field(fieldname)) {
		throw std::logic_error("Expression evaluates to type '" + exprtype.name + "' which does not have a field '" + fieldname + "'.");
	}

	return as_expression(new Dereference(std::unique_ptr<Expression>(expr), fieldname));
}

antlrcpp::Any AstBuilder::visitExprBinary(cola::CoLaParser::ExprBinaryContext* context) {
	auto op = context->binop()->accept(this).as<BinaryExpression::Operator>();
	bool is_logic = op == BinaryExpression::Operator::AND || op == BinaryExpression::Operator::OR;
	bool is_equality = op == BinaryExpression::Operator::EQ || op == BinaryExpression::Operator::NEQ;

	auto lhs = context->lhs->accept(this).as<Expression*>();
	auto lhsType = lhs->type();

	auto rhs = context->rhs->accept(this).as<Expression*>();
	auto rhsType = rhs->type();

	if (!comparable(lhsType, rhsType)) {
		throw std::logic_error("Type error: cannot compare types '" + lhsType.name + "' and " + rhsType.name + "' using operator '" + toString(op) + "'.");
	}
	if (lhsType.sort == Sort::PTR && !is_equality) {
		throw std::logic_error("Type error: pointer types allow only for (in)equality comparison.");
	}
	if (is_logic && lhsType.sort != Sort::BOOL) {
		throw std::logic_error("Type error: logic operators require bool type.");
	}

	return as_expression(new BinaryExpression(op, std::unique_ptr<Expression>(lhs), std::unique_ptr<Expression>(rhs)));
}

antlrcpp::Any AstBuilder::visitExprCas(cola::CoLaParser::ExprCasContext* context) {
	return as_expression(context->cas()->accept(this).as<CompareAndSwap*>());
}

antlrcpp::Any AstBuilder::visitInvExpr(cola::CoLaParser::InvExprContext* context) {
	Invariant* result = new InvariantExpression(std::unique_ptr<Expression>(context->expr->accept(this).as<Expression*>()));
	return result;
}

antlrcpp::Any AstBuilder::visitInvActive(cola::CoLaParser::InvActiveContext* context) {
	Invariant* result = new InvariantActive(std::unique_ptr<Expression>(context->expr->accept(this).as<Expression*>()));
	return result;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

antlrcpp::Any AstBuilder::visitBlockStmt(cola::CoLaParser::BlockStmtContext* context) {
	Statement* stmt = context->statement()->accept(this);
	Scope* scope = new Scope(std::unique_ptr<Statement>(stmt));
	return scope;
}

antlrcpp::Any AstBuilder::visitBlockScope(cola::CoLaParser::BlockScopeContext* context) {
	Scope* scope = context->scope()->accept(this).as<Scope*>();
	return scope;
}

static Statement* as_statement(Statement* stmt) {
	return stmt;
}

antlrcpp::Any AstBuilder::visitStmtIf(cola::CoLaParser::StmtIfContext* context) {
	auto ifExpr = std::unique_ptr<Expression>(context->expr->accept(this).as<Expression*>());
	auto ifScope = std::unique_ptr<Scope>(context->bif->accept(this).as<Scope*>());
	std::unique_ptr<Scope> elseScope;
	if (context->belse) {
		elseScope = std::unique_ptr<Scope>(context->belse->accept(this).as<Scope*>());
	} else {
		elseScope = std::make_unique<Scope>(std::make_unique<Skip>());
	}
	assert(elseScope);
	auto result = new IfThenElse(std::move(ifExpr), std::move(ifScope), std::move(elseScope));
	if (context->annotation()) {
		result->annotation = std::unique_ptr<Invariant>(context->annotation()->accept(this).as<Invariant*>());
	}
	return as_statement(result);
}

antlrcpp::Any AstBuilder::visitStmtWhile(cola::CoLaParser::StmtWhileContext* context) {
	bool was_in_loop = _inside_loop;
	_inside_loop = true;
	auto expr = std::unique_ptr<Expression>(context->expr->accept(this).as<Expression*>());
	auto scope = std::unique_ptr<Scope>(context->body->accept(this).as<Scope*>());
	auto result = new While(std::move(expr), std::move(scope));
	if (context->annotation()) {
		result->annotation = std::unique_ptr<Invariant>(context->annotation()->accept(this).as<Invariant*>());
	}
	_inside_loop = was_in_loop;
	return as_statement(result);
}

antlrcpp::Any AstBuilder::visitStmtDo(cola::CoLaParser::StmtDoContext* /*context*/) {
	throw std::logic_error("Unsupported construct: do-while not supported, use regular while loop instead.");
}

antlrcpp::Any AstBuilder::visitStmtChoose(cola::CoLaParser::StmtChooseContext* context) {
	auto result = new Choice();
	result->branches.reserve(context->scope().size());
	for (auto scope : context->scope()) {
		result->branches.push_back(std::unique_ptr<Scope>(scope->accept(this).as<Scope*>()));
	}
	return as_statement(result);
}

antlrcpp::Any AstBuilder::visitStmtLoop(cola::CoLaParser::StmtLoopContext* context) {
	bool was_in_loop = _inside_loop;
	_inside_loop = true;
	auto scope = std::unique_ptr<Scope>(context->scope()->accept(this).as<Scope*>());
	_inside_loop = was_in_loop;
	return as_statement(new Loop(std::move(scope)));
}

antlrcpp::Any AstBuilder::visitStmtAtomic(cola::CoLaParser::StmtAtomicContext* context) {
	auto scope = std::unique_ptr<Scope>(context->body->accept(this).as<Scope*>());
	auto result = new Atomic(std::move(scope));
	if (context->annotation()) {
		result->annotation = std::unique_ptr<Invariant>(context->annotation()->accept(this).as<Invariant*>());
	}
	return as_statement(result);
}

antlrcpp::Any AstBuilder::visitStmtCom(cola::CoLaParser::StmtComContext* context) {
	auto result = context->command()->accept(this).as<Statement*>();
	if (context->annotation()) {
		_cmdInvariant = std::unique_ptr<Invariant>(context->annotation()->accept(this).as<Invariant*>());
	}
	return as_statement(result);
}

antlrcpp::Any AstBuilder::visitAnnotation(cola::CoLaParser::AnnotationContext* context) {
	Invariant* invariant = context->invariant()->accept(this).as<Invariant*>();
	return invariant;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Statement* AstBuilder::as_command(Statement* stmt, AnnotatedStatement* cmd) {
	cmd->annotation = std::move(_cmdInvariant);
	_cmdInvariant.reset();
	assert(!_cmdInvariant);
	return stmt;
}

antlrcpp::Any AstBuilder::visitCmdSkip(cola::CoLaParser::CmdSkipContext* /*context*/) {
	return as_command(new Skip());
}

antlrcpp::Any AstBuilder::visitCmdAssign(cola::CoLaParser::CmdAssignContext* context) {
	auto lhs = std::unique_ptr<Expression>(context->lhs->accept(this).as<Expression*>());
	auto rhs = std::unique_ptr<Expression>(context->rhs->accept(this).as<Expression*>());
	if (!assignable(lhs->type(), rhs->type())) {
		throw std::logic_error("Type error: cannot assign to expression of type '" + lhs->type().name + "' from expression of type '" + rhs->type().name + "'.");
	}
	if (lhs->type() == Type::void_type()) {
		throw std::logic_error("Type error: cannot assign to 'null'.");
	}
	return as_command(new Assignment(std::move(lhs), std::move(rhs)));
}

antlrcpp::Any AstBuilder::visitCmdMalloc(cola::CoLaParser::CmdMallocContext* context) {
	auto name = context->lhs->getText();
	auto& lhs = lookupVariable(name);
	if (lhs.type.sort != Sort::PTR) {
		throw std::logic_error("Type error: cannot assign to expression of non-pointer type.");
	}
	if (lhs.type == Type::void_type()) {
		throw std::logic_error("Type error: cannot assign to 'null'.");
	}
	return as_command(new Malloc(lhs));
}

antlrcpp::Any AstBuilder::visitCmdAssume(cola::CoLaParser::CmdAssumeContext* context) {
	auto expr = std::unique_ptr<Expression>(context->expr->accept(this).as<Expression*>());
	return as_command(new Assume(std::move(expr)));
}

antlrcpp::Any AstBuilder::visitCmdAssert(cola::CoLaParser::CmdAssertContext* context) {
	auto inv = std::unique_ptr<Invariant>(context->expr->accept(this).as<Invariant*>());
	return as_command(new Assert(std::move(inv)));
}

antlrcpp::Any AstBuilder::visitCmdCall(cola::CoLaParser::CmdCallContext* context) {
	std::string name = context->name->getText();
	if (!_functions.count(name)) {
		throw std::logic_error("Error: call to undeclared function '" + name + "'.");
	}

	const Function& function = _functions.at(name);
	if (function.kind == Function::INTERFACE) {
		throw std::logic_error("Error: call to interface function '" + name + "' from interface function '" + _currentFunction->name + "', can only call 'inline'/'extern' functions.");
	}

	std::vector<std::unique_ptr<Expression>> args;
	auto argsraw = context->argList()->accept(this).as<std::vector<Expression*>>();
	args.reserve(argsraw.size());
	for (Expression* expr : argsraw) {
		args.push_back(std::unique_ptr<Expression>(expr));
	}

	if (function.args.size() != args.size()) {
		std::stringstream msg;
		msg << "Candidate function not applicable: '" << function.name << "' requires " << function.args.size() << " arguments, ";
		msg << args.size() << " arguments provided.";
		throw std::logic_error(msg.str());
	}
	for (std::size_t i = 0; i < args.size(); i++) {
		auto type_is = function.args.at(i)->type;
		auto type_required = function.args.at(i)->type;
		if (!assignable(type_required, type_is)) {
			std::stringstream msg;
			msg << "Type error: function '" << function.name << "' requires '" << type_required.name << "' for " << i << "th argument, '";
			msg << type_is.name << "' given.";
			throw std::logic_error(msg.str());
		}
	}

	if (function.kind == Function::SMR) {
		auto enter = std::make_unique<Enter>(function);
		enter->args = std::move(args);
		auto exit = std::make_unique<Exit>(function);
		auto seq = new Sequence(std::move(enter), std::move(exit));
		return as_command(seq, static_cast<Enter*>(seq->first.get()));

	} else if (function.kind == Function::MACRO) {
		if (function.return_type != Type::void_type()) {
			// throw std::logic_error("Unsupported inline function: cannot call non-void inline function.");
			throw std::logic_error("Compilation error: I did not expect that to happen.");
		}

		auto macro = new Macro(function);
		macro->args = std::move(args);
		return as_command(macro);

	} else {
		throw std::logic_error("Internal compilation errror: I did not expect this to happen.");
	}
}

antlrcpp::Any AstBuilder::visitArgList(cola::CoLaParser::ArgListContext* context) {
	std::vector<Expression*> results;
	results.reserve(context->arg.size());
	for (auto context : context->arg) {
		results.push_back(context->accept(this).as<Expression*>());
	}
	return results;
}

antlrcpp::Any AstBuilder::visitCmdContinue(cola::CoLaParser::CmdContinueContext* /*context*/) {
	if (!_inside_loop) {
		throw std::logic_error("Semantic error: 'continue' must only appear inside loops.");
	}
	return as_command(new Continue());
}

antlrcpp::Any AstBuilder::visitCmdBreak(cola::CoLaParser::CmdBreakContext* /*context*/) {
	if (!_inside_loop) {
		throw std::logic_error("Semantic error: 'break' must only appear inside loops.");
	}
	return as_command(new Break());
}

antlrcpp::Any AstBuilder::visitCmdReturn(cola::CoLaParser::CmdReturnContext* context) {
	if (context->expr) {
		auto expr = std::unique_ptr<Expression>(context->expr->accept(this).as<Expression*>());
		if (expr->type() != _currentFunction->return_type) {
			throw std::logic_error("Type error: return value has type '" + expr->type().name + "' but function has return type '" + _currentFunction->return_type.name + "'.");
		}
		return as_command(new Return(std::move(expr)));
	} else {
		return as_command(new Return());
	}
}

antlrcpp::Any AstBuilder::visitCmdCas(cola::CoLaParser::CmdCasContext* context) {
	return as_command(context->cas()->accept(this).as<CompareAndSwap*>());
}

antlrcpp::Any AstBuilder::visitCas(cola::CoLaParser::CasContext* context) {
	if (context->dst.size() != context->cmp.size() || context->cmp.size() != context->src.size()) {
		throw std::logic_error("Malformed CAS.");
	} 

	auto result = new CompareAndSwap();
	result->elems.reserve(context->dst.size());
	for (std::size_t i = 0; i < context->dst.size(); i++) {
		auto dst = std::unique_ptr<Expression>(context->dst.at(i)->accept(this).as<Expression*>());
		auto cmp = std::unique_ptr<Expression>(context->cmp.at(i)->accept(this).as<Expression*>());
		auto src = std::unique_ptr<Expression>(context->src.at(i)->accept(this).as<Expression*>());
		result->elems.push_back(CompareAndSwap::Triple(std::move(dst), std::move(cmp), std::move(src)));
	}

	return result;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

antlrcpp::Any AstBuilder::visitOption(cola::CoLaParser::OptionContext* context) {
	std::string key = context->ident->getText();
	std::string val = context->str->getText();
	val.erase(val.begin());
	val.pop_back();

	if (key == "name") {
		_program->name = val;
	}

	auto insertion = _program->options.insert({ key, val });

	if (!insertion.second) {
		throw std::logic_error("Multiple options with the same key are not supported.");
	}

	return nullptr;
}
