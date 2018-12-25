#include "parser/AstBuilder.hpp"

#include "parser/TypeBuilder.hpp"


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
	std::vector<std::unique_ptr<VariableDeclaration>> decls;
	decls.reserve(_scope.back().size());
	for (auto& entry : _scope.back()) {
		decls.push_back(std::move(entry.second));
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
	for (auto it = _scope.crbegin(); it != _scope.rend(); it++) {
		if (it->count(variableName)) {
			return *it->at(variableName);
		}
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

	if (!_program->initalizer) {
		throw std::logic_error("Program has not init function.");
	}

	// finish
	_program->variables = popScope();

	return nullptr;
}

antlrcpp::Any AstBuilder::visitFunction(cola::CoLaParser::FunctionContext* context) {
	std::string name = context->name->getText();
	const Type& returnType = context->returnType->accept(this).as<const Type&>();
	
	Function::Kind kind = Function::INTERFACE;
	assert(!(context->Inline() && context->Extern()));
	if (context->Inline()) {
		kind = Function::MACRO;
	} else if (context->Extern()) {
		kind = Function::SMR;
	}

	if (kind == Function::SMR && returnType != Type::void_type()) {
		throw std::logic_error("Return type of SMR function " + name + " not support: must be of type 'void'.");
	}

	if (name == INIT_NAME) {
		if (kind != Function::INTERFACE) {
			throw std::logic_error("Initalizer function '" + name + "' must not have modifier.");
		}
		if (returnType != Type::void_type()) {
			throw std::logic_error("Initalizer function '" + name + "' must have 'void' return type.");
		}
	}

	auto function = std::make_unique<Function>(name, returnType, kind);
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
	auto body = context->body->accept(this).as<Scope*>();
	function->body = std::unique_ptr<Scope>(body);

	// get variable decls
	function->args = popScope();

	// transfer ownershp
	if (name != INIT_NAME) {
		_program->initalizer = std::move(function);
	} else {
		_program->functions.push_back(std::move(function));
	}

	return nullptr;
}

antlrcpp::Any AstBuilder::visitArgDeclList(cola::CoLaParser::ArgDeclListContext* context) {
	auto dlist = new ArgDeclList(); //std::make_shared<ArgDeclList>();
	auto size = context->argTypes.size();
	dlist->reserve(size);
	assert(size == context->argNames.size());
	for (std::size_t i = 0; i < size; i++) {
		std::string name = context->argNames.at(i)->getText();
		const Type& type = context->argTypes.at(i)->accept(this).as<const Type&>();
		// TODO: swhat the ****?! why can't I pass the reference wrapper here without them being lost after the loop?
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
	return lookupType(name);
}

antlrcpp::Any AstBuilder::visitTypePointer(cola::CoLaParser::TypePointerContext* context) {
	std::string name = context->name->accept(this).as<std::string>() + "*";
	return lookupType(name);
}

antlrcpp::Any AstBuilder::visitVar_decl(cola::CoLaParser::Var_declContext* context) {
	const Type& type = context->type()->accept(this).as<const Type&>();
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
	return as_expression(new NegatedExpresion(std::unique_ptr<Expression>(expr)));
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
	throw std::logic_error("not yet implemented (visitExprCas)");
}

antlrcpp::Any AstBuilder::visitInvExpr(cola::CoLaParser::InvExprContext* context) {
	// TODO: return as Invariant*
	throw std::logic_error("not yet implemented (visitInvExpr)");
}

antlrcpp::Any AstBuilder::visitInvActive(cola::CoLaParser::InvActiveContext* context) {
	// TODO: return as Invariant*
	throw std::logic_error("not yet implemented (visitInvActive)");
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static Statement* as_statement(Statement* stmt) {
	return stmt;
}

antlrcpp::Any AstBuilder::visitBlockStmt(cola::CoLaParser::BlockStmtContext* context) {
	return as_statement(context->statement()->accept(this));
}

antlrcpp::Any AstBuilder::visitBlockScope(cola::CoLaParser::BlockScopeContext* context) {
	return as_statement(context->scope()->accept(this).as<Scope*>());
}

antlrcpp::Any AstBuilder::visitStmtIf(cola::CoLaParser::StmtIfContext* context) {
	// TODO: avoid wrapping a scope into a scope
	auto ifExpr = std::unique_ptr<Expression>(context->expr->accept(this).as<Expression*>());
	auto ifBranch = std::unique_ptr<Statement>(context->bif->accept(this).as<Statement*>());
	auto ifScope = std::make_unique<Scope>(std::move(ifBranch));
	std::unique_ptr<Scope> elseScope;
	if (context->belse) {
		auto elseBranch = std::unique_ptr<Statement>(context->belse->accept(this).as<Statement*>());
		elseScope = std::make_unique<Scope>(std::move(elseBranch));
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
	// TODO: avoid wrapping a scope into a scope
	auto expr = std::unique_ptr<Expression>(context->expr->accept(this).as<Expression*>());
	auto body = std::unique_ptr<Statement>(context->body->accept(this).as<Statement*>());
	auto scope = std::make_unique<Scope>(std::move(body));
	auto result = new While(std::move(expr), std::move(scope));
	if (context->annotation()) {
		result->annotation = std::unique_ptr<Invariant>(context->annotation()->accept(this).as<Invariant*>());
	}
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
	auto scope = std::unique_ptr<Scope>(context->scope()->accept(this).as<Scope*>());
	return as_statement(new Loop(std::move(scope)));
}

antlrcpp::Any AstBuilder::visitStmtAtomic(cola::CoLaParser::StmtAtomicContext* context) {
	auto body = std::unique_ptr<Statement>(context->body->accept(this).as<Statement*>());
	auto scope = std::make_unique<Scope>(std::move(body)); // TODO: avoid wrapping a scope into a scope
	auto result = new Atomic(std::move(scope));
	if (context->annotation()) {
		result->annotation = std::unique_ptr<Invariant>(context->annotation()->accept(this).as<Invariant*>());
	}
	return as_statement(result);
}

antlrcpp::Any AstBuilder::visitStmtCom(cola::CoLaParser::StmtComContext* context) {
	auto result = context->command()->accept(this).as<Command*>();
	if (context->annotation()) {
		result->annotation = std::unique_ptr<Invariant>(context->annotation()->accept(this).as<Invariant*>());
	}
	return as_statement(result);
}

antlrcpp::Any AstBuilder::visitAnnotation(cola::CoLaParser::AnnotationContext* context) {
	// TODO: handle annotation in statements
	throw std::logic_error("not yet implemented (visitAnnotation)");
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static Command* as_command(Command* cmd) {
	return cmd;
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

antlrcpp::Any AstBuilder::visitCmdMallo(cola::CoLaParser::CmdMalloContext* context) {
	auto name = context->lhs->getText();
	auto lhs = lookupVariable(name);
	if (lhs.type.sort != Sort::PTR) {
		throw std::logic_error("Type error: cannot assign to expression of non-pointer type.");
	}
	if (lhs.type == Type::void_type()) {
		throw std::logic_error("Type error: cannot assign to 'null'.");
	}
	return as_command(new Malloc(std::move(lhs)));
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
	throw std::logic_error("not yet implemented (visitCmdCall)");
}

antlrcpp::Any AstBuilder::visitCmdContinue(cola::CoLaParser::CmdContinueContext* /*context*/) {
	if (!_inside_loop) {
		throw std::logic_error("Semantic error: 'continue' must only appear inside loops.");
	}
	return as_command(new Continue());
}

antlrcpp::Any AstBuilder::visitCmdBreak(cola::CoLaParser::CmdBreakContext* context) {
	if (!_inside_loop) {
		throw std::logic_error("Semantic error: 'break' must only appear inside loops.");
	}
	return as_command(new Break());
}

antlrcpp::Any AstBuilder::visitCmdReturn(cola::CoLaParser::CmdReturnContext* context) {
	throw std::logic_error("not yet implemented (visitCmdReturn)");
}

antlrcpp::Any AstBuilder::visitCmdCas(cola::CoLaParser::CmdCasContext* context) {
	throw std::logic_error("not yet implemented (visitCmdCas)");
}

antlrcpp::Any AstBuilder::visitArgList(cola::CoLaParser::ArgListContext* context) {
	throw std::logic_error("not yet implemented (visitArgList)");
}

antlrcpp::Any AstBuilder::visitCas(cola::CoLaParser::CasContext* context) {
	throw std::logic_error("not yet implemented (visitCas)");
}
















