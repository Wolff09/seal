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

	// _scope.back().insert({{ variable->name, std::move(variable) }});
	_scope.back()[variable->name] = std::move(variable);
}

const VariableDeclaration& AstBuilder::makeAuxVar(const Type& type) {
	std::string name = "_aux" + std::to_string(_tmpCounter++); // should not name clash due to underscore
	addVariable(std::make_unique<VariableDeclaration>(name, type, false));
	return lookupVariable(name);
}

bool AstBuilder::isTypeDeclared(std::string typeName) {
	return _types.count(typeName) != 0;
}

const Type& AstBuilder::lookupType(std::string typeName) {
	auto it = _types.find(typeName);
	if (it == _types.end()) {
		throw std::logic_error("Type '" + typeName + "' not declared.");
	} else {
		return it->second;
	}
}

std::unique_ptr<Statement> AstBuilder::mk_stmt_from_list(std::vector<cola::CoLaParser::StatementContext*> context) {
	// TODO: implement (reduce to vector version)
	throw std::logic_error("Not yet implemented (mk_stmt_from_list).");
}

std::unique_ptr<Statement> mk_stmt_from_list(std::vector<Statement*> stmts) {
	// TODO: implement (care for empty vector => skip command)
	throw std::logic_error("Not yet implemented (mk_stmt_from_list).");
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
	// return parseTree->accept(&builder).as<std::shared_ptr<Program>>();
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
	pushScope();

	if (_functions.count(name)) {
		throw std::logic_error("Duplicate function declaration: function with name '" + name + "' already defined.");
	}
	_functions.insert({{ name, *function }});

	// add arguments
	auto arglist = context->args->accept(this).as<ArgDeclList>();
	function->variables.reserve(arglist.size() + context->var_decl().size());
	for (auto& pair : arglist) {
		std::string argname = pair.first;
		const Type& argtype = pair.second;
		auto decl = std::make_unique<VariableDeclaration>(argname, argtype, false);
		addVariable(std::move(decl));

		if (argtype.sort == Sort::VOID) {
			throw std::logic_error("Argument type 'void' not supported for argument '" + argname + "'.");
		}
		if (kind == Function::INTERFACE && argtype.sort == Sort::PTR) {
			throw std::logic_error("Argument with name '" + argname + "' of pointer sort not supported for interface functions.");
		}
	}

	// add local variables
	auto vars = context->var_decl();
	for (auto varDeclContext : vars) {
		varDeclContext->accept(this);
	}

	// handle body
	auto stmts = context->statement();
	if (kind == Function::SMR) {
		if (stmts.size() != 0 || vars.size() != 0) {
			throw std::logic_error("Functions with 'extern' modifier must not have an implementation.");
		}
	} else {
		function->body = mk_stmt_from_list(stmts);
	}

	// get variable decls
	function->variables = popScope();
	_program->functions.push_back(std::move(function));

	return nullptr;
}

antlrcpp::Any AstBuilder::visitArgDeclList(cola::CoLaParser::ArgDeclListContext *context) {
	throw std::logic_error("not yet implemented (visitArgDeclList)");
}

antlrcpp::Any AstBuilder::visitStruct_decl(cola::CoLaParser::Struct_declContext* /*context*/) {
	throw std::logic_error("Unsupported operation.");
}

antlrcpp::Any AstBuilder::visitField_decl(cola::CoLaParser::Field_declContext* /*context*/) {
	throw std::logic_error("Unsupported operation.");
}

antlrcpp::Any AstBuilder::visitNameVoid(cola::CoLaParser::NameVoidContext* /*context*/) {
	return "void";
}

antlrcpp::Any AstBuilder::visitNameBool(cola::CoLaParser::NameBoolContext* /*context*/) {
	return "bool";
}

antlrcpp::Any AstBuilder::visitNameInt(cola::CoLaParser::NameIntContext* /*context*/) {
	throw std::logic_error("Type 'int' not supported. Use 'data_t' instead.");
}

antlrcpp::Any AstBuilder::visitNameData(cola::CoLaParser::NameDataContext* /*context*/) {
	return "data_t";
}

antlrcpp::Any AstBuilder::visitNameIdentifier(cola::CoLaParser::NameIdentifierContext* context) {
	return context->Identifier()->getText();
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
	// expr_1 || expr_2 --> block_1;block_2;b=b_1||b_2;assume(b);  with  block_i := if (expr_i) then b_i = true; else b_i = false; end
	// throw std::logic_error("Unsupported expression: cannot handle disjunctions '||'.");
	return BinaryExpression::Operator::OR;
}

antlrcpp::Any AstBuilder::visitValueNull(cola::CoLaParser::ValueNullContext* /*context*/) {
	return ExprTrans(ExprForm::PTR, new NullValue());
}

antlrcpp::Any AstBuilder::visitValueTrue(cola::CoLaParser::ValueTrueContext* /*context*/) {
	return ExprTrans(ExprForm::NOPTR, new BooleanValue(true));
}

antlrcpp::Any AstBuilder::visitValueFalse(cola::CoLaParser::ValueFalseContext* /*context*/) {
	return ExprTrans(ExprForm::NOPTR, new BooleanValue(false));
}

antlrcpp::Any AstBuilder::visitValueNDet(cola::CoLaParser::ValueNDetContext* /*context*/) {
	return ExprTrans(ExprForm::NOPTR, new NullValue());
}

antlrcpp::Any AstBuilder::visitValueEmpty(cola::CoLaParser::ValueEmptyContext* /*context*/) {
	return ExprTrans(ExprForm::NOPTR, new EmptyValue());
}

antlrcpp::Any AstBuilder::visitExprValue(cola::CoLaParser::ExprValueContext* context) {
	return context->value()->accept(this);
}

antlrcpp::Any AstBuilder::visitExprParens(cola::CoLaParser::ExprParensContext* context) {
	return context->expr->accept(this);
}

antlrcpp::Any AstBuilder::visitExprIdentifier(cola::CoLaParser::ExprIdentifierContext* context) {
	std::string name = context->name->getText();
	const VariableDeclaration& decl = lookupVariable(name);
	const Type& type = decl.type;
	ExprForm form = type.sort == Sort::PTR ? ExprForm::PTR : ExprForm::NOPTR;
	return ExprTrans(form, new VariableExpression(decl), decl.is_shared ? 1 : 0);
}

antlrcpp::Any AstBuilder::visitExprNegation(cola::CoLaParser::ExprNegationContext* /*context*/) {
	// TODO: !expr --> if (expr) then b = false else b = true end; assume(b)
	throw std::logic_error("Unsupported expression: cannot handle negations '!'.");
}

static Expression* mk_negated(Expression* expr) {
	// copy expr and negate
	throw std::logic_error("not yet implemented (mk_negated).");
}

void AstBuilder::transformIfRequired(ExprTrans& trans, ExprForm newform) {
	if (_simplify && newform == _exprSplit) {
		// TODO: split differently on trans.form == ExprForm::PTR ==> use assumes
		if (trans.form == ExprForm::PTR) {
			// translate trans.expr into: choose { assume(expr); aux = true; }{ assume(!expr); aux = false; }
			auto decl = makeAuxVar(Type::bool_type());
			auto exprF = std::unique_ptr<Expression>(mk_negated(trans.expr));
			auto exprT = std::unique_ptr<Expression>(trans.expr);
			auto choice = new Choice();
			choice->branches.reserve(2);
			choice->branches.push_back(std::make_unique<Sequence>(
				std::make_unique<Assume>(std::move(exprT)),
				std::make_unique<Assignment>(std::make_unique<VariableExpression>(decl), std::make_unique<BooleanValue>(true))
			));
			choice->branches.push_back(std::make_unique<Sequence>(
				std::make_unique<Assume>(std::move(exprF)),
				std::make_unique<Assignment>(std::make_unique<VariableExpression>(decl), std::make_unique<BooleanValue>(false))
			));
			trans.helper.push_back(choice);
			trans.expr = new VariableExpression(decl);
			trans.form = ExprForm::NOPTR;
			trans.num_shared = 0;

		} else {
			// translate trans.expr into: aux = expr
			auto decl = makeAuxVar(trans.expr->type());
			auto lhs = std::make_unique<VariableExpression>(decl);
			auto rhs = std::unique_ptr<Expression>(trans.expr);
			auto assign = new Assignment(std::move(lhs), std::move(rhs));
			trans.helper.push_back(assign);
			trans.expr = new VariableExpression(decl);
			trans.form = trans.expr->type().sort == Sort::PTR ? ExprForm::PTR : ExprForm::NOPTR;
			trans.num_shared = 0;
		}
	}
}

antlrcpp::Any AstBuilder::visitExprDeref(cola::CoLaParser::ExprDerefContext* context) {
	auto trans = context->expr->accept(this).as<ExprTrans>();
	std::string fieldname = context->field->getText();
	const Type& exprtype = trans.expr->type();

	if (exprtype.sort != Sort::PTR) {
		throw std::logic_error("Cannot dereference expression of non-pointer type.");
	}
	if (exprtype == Type::null_type()) {
		throw std::logic_error("Cannot dereference 'nullptr'.");
	}
	if (!exprtype.has_field(fieldname)) {
		throw std::logic_error("Expression evaluates to type '" + exprtype.name + "' which does not have a field '" + fieldname + "'.");
	}
	if (trans.form == ExprForm::NOPTR) {
		throw std::logic_error("Compilation error: did not expect non-pointer type here.");
	}

	auto newform = [](ExprForm form) -> ExprForm {
		return form == ExprForm::PTR ? ExprForm::DEREF : ExprForm::NESTED;
	};

	transformIfRequired(trans, newform(trans.form));
	trans.expr = new Dereference(std::unique_ptr<Expression>(trans.expr), fieldname);
	trans.form = newform(trans.form);
	return trans;
}

antlrcpp::Any AstBuilder::visitExprBinary(cola::CoLaParser::ExprBinaryContext* context) {
	// TODO: how to reuse auxiliary variables?
	auto op = context->binop()->accept(this).as<BinaryExpression::Operator>();

	bool is_logic = op == BinaryExpression::Operator::AND || op == BinaryExpression::Operator::OR;
	bool is_equality = op == BinaryExpression::Operator::EQ || op == BinaryExpression::Operator::NEQ;
	ExprForm toSplit = is_logic ? ExprForm::PTR : ExprForm::DEREF;

	_exprSplit = toSplit;
	auto left = context->lhs->accept(this).as<ExprTrans>();
	const Type& leftType = left.expr->type();
	_exprSplit = toSplit;
	transformIfRequired(left, left.form);

	_exprSplit = toSplit;
	auto right = context->rhs->accept(this).as<ExprTrans>();
	const Type& rightType = right.expr->type();
	_exprSplit = toSplit;
	transformIfRequired(right, right.form);

	if (!comparable(leftType, rightType)) {
		throw std::logic_error("Type error: cannot compare types '" + leftType.name + "' and " + rightType.name + "' using operator '" + toString(op) + "'.");
	}
	if (leftType.sort == Sort::PTR && !is_equality) {
		throw std::logic_error("Type error: pointer types allow only for (in)equality comparison.");
	}
	if (is_logic && leftType.sort != Sort::BOOL) {
		throw std::logic_error("Type error: logic operators require bool type.");
	}

	ExprTrans trans;
	trans.form = left.form != ExprForm::NOPTR || right.form != ExprForm::NOPTR ? ExprForm::PTR : ExprForm::NOPTR;
	trans.helper.reserve(left.helper.size() + right.helper.size());
	for (Statement* stmt : left.helper) {
		trans.helper.push_back(stmt);
	}
	for (Statement* stmt : right.helper) {
		trans.helper.push_back(stmt);
	}
	trans.expr = new BinaryExpression(op, std::unique_ptr<Expression>(left.expr), std::unique_ptr<Expression>(right.expr));
	trans.num_shared = 0;
	transformIfRequired(trans, trans.form);
	return trans;
}

antlrcpp::Any AstBuilder::visitExprCas(cola::CoLaParser::ExprCasContext* context) {
	throw std::logic_error("not yet implemented (visitExprCas)");
}

antlrcpp::Any AstBuilder::visitInvExpr(cola::CoLaParser::InvExprContext* context) {
	throw std::logic_error("not yet implemented (visitInvExpr)");
}

antlrcpp::Any AstBuilder::visitInvActive(cola::CoLaParser::InvActiveContext* context) {
	throw std::logic_error("not yet implemented (visitInvActive)");
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

antlrcpp::Any AstBuilder::visitBlockStmt(cola::CoLaParser::BlockStmtContext* context) {
	throw std::logic_error("not yet implemented (visitBlockStmt)");
}

antlrcpp::Any AstBuilder::visitBlockBlock(cola::CoLaParser::BlockBlockContext* context) {
	throw std::logic_error("not yet implemented (visitBlockBlock)");
}

antlrcpp::Any AstBuilder::visitStmtIf(cola::CoLaParser::StmtIfContext* context) {
	throw std::logic_error("not yet implemented (visitStmtIf)");
}

antlrcpp::Any AstBuilder::visitStmtWhile(cola::CoLaParser::StmtWhileContext* context) {
	throw std::logic_error("not yet implemented (visitStmtWhile)");
}

antlrcpp::Any AstBuilder::visitStmtDo(cola::CoLaParser::StmtDoContext* context) {
	throw std::logic_error("not yet implemented (visitStmtDo)");
}

antlrcpp::Any AstBuilder::visitStmtChoose(cola::CoLaParser::StmtChooseContext* context) {
	throw std::logic_error("not yet implemented (visitStmtChoose)");
}

antlrcpp::Any AstBuilder::visitStmtLoop(cola::CoLaParser::StmtLoopContext* context) {
	throw std::logic_error("not yet implemented (visitStmtLoop)");
}

antlrcpp::Any AstBuilder::visitStmtAtomic(cola::CoLaParser::StmtAtomicContext* context) {
	throw std::logic_error("not yet implemented (visitStmtAtomic)");
}

antlrcpp::Any AstBuilder::visitStmtCom(cola::CoLaParser::StmtComContext* context) {
	throw std::logic_error("not yet implemented (visitStmtCom)");
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

antlrcpp::Any AstBuilder::visitCmdSkip(cola::CoLaParser::CmdSkipContext* context) {
	throw std::logic_error("not yet implemented (visitCmdSkip)");
}

antlrcpp::Any AstBuilder::visitCmdAssign(cola::CoLaParser::CmdAssignContext* context) {
	throw std::logic_error("not yet implemented (visitCmdAssign)");
}

antlrcpp::Any AstBuilder::visitCmdMallo(cola::CoLaParser::CmdMalloContext* context) {
	throw std::logic_error("not yet implemented (visitCmdMallo)");
}

antlrcpp::Any AstBuilder::visitCmdAssume(cola::CoLaParser::CmdAssumeContext* context) {
	throw std::logic_error("not yet implemented (visitCmdAssume)");
}

antlrcpp::Any AstBuilder::visitCmdAssert(cola::CoLaParser::CmdAssertContext* context) {
	throw std::logic_error("not yet implemented (visitCmdAssert)");
}

antlrcpp::Any AstBuilder::visitCmdInvariant(cola::CoLaParser::CmdInvariantContext* context) {
	throw std::logic_error("not yet implemented (visitCmdInvariant)");
}

antlrcpp::Any AstBuilder::visitCmdCall(cola::CoLaParser::CmdCallContext* context) {
	throw std::logic_error("not yet implemented (visitCmdCall)");
}

antlrcpp::Any AstBuilder::visitCmdContinue(cola::CoLaParser::CmdContinueContext* context) {
	throw std::logic_error("not yet implemented (visitCmdContinue)");
}

antlrcpp::Any AstBuilder::visitCmdBreak(cola::CoLaParser::CmdBreakContext* context) {
	throw std::logic_error("not yet implemented (visitCmdBreak)");
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
















