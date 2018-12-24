// #include "parser/AstBuilder.hpp"

// // TODO: include assert

// // using namespace antlr4;
// using namespace antlrcpptest;

// struct VarOrVal {
// 	public:
// 		enum Sort { VAR, VARADDONE, VAL };
// 	private:
// 		Sort _sort;
// 		std::shared_ptr<VariableDeclaration> _variable;
// 		Immediate _immediate;
// 	public:
// 		VarOrVal(std::shared_ptr<VariableDeclaration> variable, bool addone=false) : _sort(addone ? VARADDONE : VAR), _variable(variable) {
// 			if (addone && variable->type != Type::TAG) {
// 				throw std::logic_error("Cannot add to non-tag type.");
// 			}
// 		}
// 		VarOrVal(Immediate immediate) : _sort(VAL), _immediate(immediate) {}
// 		Sort sort() const { return _sort; }
// 		std::shared_ptr<VariableDeclaration> variable() const {
// 			assert(_sort == VAR || _sort == VARADDONE);
// 			return _variable;
// 		}
// 		Immediate immediate() const {
// 			assert(_sort == VAL);
// 			return _immediate;
// 		}
// 		Type type() const {
// 			switch (sort()) {
// 				case VAR:
// 				case VARADDONE:
// 					return variable()->type;
// 				case VAL:
// 					return typeOf(immediate());
// 			}
// 	}
// };


// void AstBuilder::pushScope(bool collapsible) {
// 	// pushes a new scope; if collapsible=true, then the new scope will
// 	// automatically be merged with the nexted scope pushed.
// 	if (!_lastScopeIsCollapsible) {
// 		_scope.push_back(VariableMap());
// 	}
// 	_lastScopeIsCollapsible = collapsible;
// }

// void AstBuilder::popScope() {
// 	// pops the latest added scope
// 	_scope.pop_back();
// 	_lastScopeIsCollapsible = false;
// }

// std::shared_ptr<VariableDeclaration> AstBuilder::lookupVariable(std::string variableName) {
// 	for (auto it = _scope.crbegin(); it != _scope.rend(); it++) {
// 		if (it->count(variableName)) {
// 			return it->at(variableName);
// 		}
// 	}
// 	return nullptr;
// }

// std::shared_ptr<VariableDeclaration> AstBuilder::lookupVariableOrFail(std::string variableName) {
// 	auto declaration = lookupVariable(variableName);
// 	if (declaration == nullptr) {
// 		// TODO: proper error class
// 		throw std::logic_error("Variable '" + variableName + "' not found.");
// 	} else {
// 		return declaration;
// 	}
// }

// void AstBuilder::addVariable(std::shared_ptr<VariableDeclaration> variable) {
// 	if (lookupVariable(variable->name)) {
// 		// TODO: proper error class
// 		throw std::logic_error("CompilationError: variable '" + variable->name + "' already declared.");
// 	}

// 	_scope.back().insert({{ variable->name, variable }});
// }

// std::vector<std::shared_ptr<VariableDeclaration>> AstBuilder::copyVariablesFromCurrentScope() {
// 	std::vector<std::shared_ptr<VariableDeclaration>> result;
// 	result.reserve(_scope.back().size());
// 	for (auto entry : _scope.back()) {
// 		result.push_back(entry.second);
// 	}
// 	return result;
// }


// std::shared_ptr<Program> AstBuilder::buildFrom(PexlParser::ProgramContext* parseTree) {
// 	AstBuilder builder;
// 	parseTree->accept(&builder);
// 	return builder._program;
// 	// return parseTree->accept(&builder).as<std::shared_ptr<Program>>();
// }

// antlrcpp::Any AstBuilder::visitProgram(PexlParser::ProgramContext* context) {
// 	_program = std::make_shared<Program>();
// 	pushScope();
	
// 	// options
// 	for (auto option : context->option()) {
// 		option->accept(this);
// 	}

// 	// shared variables
// 	for (auto variableContext : context->var_decl()) {
// 		variableContext->accept(this);
// 	}
// 	_program->variables = copyVariablesFromCurrentScope();
// 	for (auto variable : _program->variables) {
// 		variable->is_shared = true;
// 		std::cout << "Variable: " << variable->name << std::endl;
// 	}

// 	// functions/summaries
// 	_program->functions.reserve(context->function().size());
// 	for (auto functionContext : context->function()) {
// 		auto function = functionContext->accept(this).as<std::shared_ptr<Function>>();
// 		_program->functions.push_back(function);
// 	}

// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionName(PexlParser::OptionNameContext* context) {
// 	_program->name = context->name->getText();
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionLinearizable(PexlParser::OptionLinearizableContext* /*context*/) {
// 	_program->setup.linearizable = true;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionNotLinearizable(PexlParser::OptionNotLinearizableContext* /*context*/) {
// 	_program->setup.linearizable = false;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionTaggedPointers(PexlParser::OptionTaggedPointersContext* /*context*/) {
// 	_program->setup.use_tagged_pointers = true;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionMarkedPointers(PexlParser::OptionMarkedPointersContext* /*context*/) {
// 	_program->setup.use_marked_pointers = true;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionGC(PexlParser::OptionGCContext* /*context*/) {
// 	_program->setup.semantics = Semantics::GC;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionMM(PexlParser::OptionMMContext* /*context*/) {
// 	_program->setup.semantics = Semantics::MM;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionStack(PexlParser::OptionStackContext* /*context*/) {
// 	_program->setup.container = Container::STACK;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionQueue(PexlParser::OptionQueueContext* /*context*/) {
// 	_program->setup.container = Container::QUEUE;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionSet(PexlParser::OptionSetContext* /*context*/) {
// 	_program->setup.container = Container::SET;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionHazardPointers(PexlParser::OptionHazardPointersContext* /*context*/) {
// 	_program->setup.reclamation = Reclamation::HAZARD;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionEpochBased(PexlParser::OptionEpochBasedContext* /*context*/) {
// 	_program->setup.reclamation = Reclamation::EPOCH;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitOptionFreeList(PexlParser::OptionFreeListContext* /*context*/) {
// 	_program->setup.reclamation = Reclamation::FREE;
// 	return nullptr;
// }

// antlrcpp::Any AstBuilder::visitTypePointer(PexlParser::TypePointerContext* /*context*/) {
// 	return Type::RAW_POINTER;
// }

// antlrcpp::Any AstBuilder::visitTypeBool(PexlParser::TypeBoolContext* /*context*/) {
// 	return Type::BOOL;
// }

// antlrcpp::Any AstBuilder::visitTypeTag(PexlParser::TypeTagContext* /*context*/) {
// 	return Type::TAG;
// }

// antlrcpp::Any AstBuilder::visitTypeData(PexlParser::TypeDataContext* /*context*/) {
// 	return Type::DATA;
// }

// antlrcpp::Any AstBuilder::visitTypeVoid(PexlParser::TypeVoidContext* /*context*/) {
// 	return Type::VOID;
// }

// antlrcpp::Any AstBuilder::visitVariableDeclaration(PexlParser::VariableDeclarationContext *context) {
// 	auto vartype = context->type()->accept(this).as<Type>();

// 	if (vartype == Type::VOID) {
// 		// TODO: proper error class
// 		throw std::logic_error("Variable must not be of 'void' type.");
// 	}

// 	auto result = std::make_shared<VariableDeclaration>();
// 	result->name = context->name->getText();
// 	result->type = vartype;
// 	addVariable(result);

// 	return nullptr;
// }

// static void check_function_return_type(Type type) {
// 	if (!(type == Type::DATA || type == Type::BOOL || type == Type::VOID)) {
// 		// TODO: proper error class
// 		throw std::logic_error("Function must return type must be one of: void, bool, data.");
// 	}
// }

// static void check_function_parameter_type(Type type) {
// 	if (type != Type::DATA) {
// 		// TODO: proper error class
// 		throw std::logic_error("Function parameter must be of data type.");
// 	}
// }

// antlrcpp::Any AstBuilder::visitFunction(PexlParser::FunctionContext* context) {
// 	auto result = std::make_shared<Function>();
// 	_currentFunction = result;
	
// 	result->program = _program;
// 	result->name = context->name->getText();
// 	result->is_summary = context->isSummary != nullptr;
// 	result->return_type = context->returnType->accept(this).as<Type>();
// 	check_function_return_type(result->return_type);

// 	auto has_parameter = context->paramName && context->paramType;
// 	if (has_parameter) {
// 		auto parameter = std::make_shared<VariableDeclaration>();
// 		parameter->name = context->paramName->getText();
// 		parameter->type = context->paramType->accept(this).as<Type>();
// 		check_function_parameter_type(parameter->type);
// 		parameter->is_shared = false;

// 		pushScope(true);
// 		addVariable(parameter);
// 	}

// 	auto body = context->block()->accept(this).as<std::shared_ptr<Block>>();
// 	result->body = body;
// 	// TODO: add statement that initializes parameter variable

// 	assert(body != nullptr);
// 	assert(!_lastScopeIsCollapsible);
// 	_currentFunction = nullptr;
// 	return result;
// }

// template<typename T>
// std::shared_ptr<T> AstBuilder::makeBaseStatement() {
// 	auto result = std::make_shared<T>();
// 	result->program = _program;
// 	result->function = _currentFunction;
// 	result->block = _currentBlock;
// 	result->linearization_point = nullptr;
// 	return result;
// }

// template<typename T>
// static std::shared_ptr<Statement> asBaseStatement(T stmt) {
// 	return static_cast<std::shared_ptr<Statement>>(stmt);
// }

// std::shared_ptr<AssumeStatement> AstBuilder::makeAssumeFromCondition(std::shared_ptr<Condition> condition, bool negateCondition) {
// 	auto result = makeBaseStatement<AssumeStatement>();
// 	auto resultCondition = condition;
// 	if (negateCondition) {
// 		throw std::logic_error("Not yet implemented (negating conditions)");
// 	}
// 	result->condition = resultCondition;
// 	return result;
// }

// antlrcpp::Any AstBuilder::visitBlock(PexlParser::BlockContext* context) {
// 	auto result = makeBaseStatement<Block>();

// 	// preamble
// 	auto parentBlock = _currentBlock;
// 	_currentBlock = result;
// 	pushScope();

// 	// local variables
// 	for (auto variableContext : context->var_decl()) {
// 		variableContext->accept(this);
// 	}
// 	result->variables = copyVariablesFromCurrentScope();
// 	for (auto variable : result->variables) {
// 		variable->is_shared = false;
// 		std::cout << "Variable: " << variable->name << std::endl;
// 	}

// 	// statements
// 	result->statements.reserve(context->statement().size());
// 	for (auto statementContext : context->statement()) {
// 		// auto statement = statementContext->accept(this).as<std::shared_ptr<Statement>>();
// 		auto tmp = statementContext->accept(this);
// 		std::cout << "@visitBlock: trying to cast statement?" << std::endl;
// 		auto statement = tmp.as<std::shared_ptr<Statement>>();
// 		result->statements.push_back(statement);
// 	}

// 	// epiloge
// 	popScope();
// 	_currentBlock = parentBlock;
	
// 	// do not return asBaseStatement here; this visitor is not in the statement hierachy
// 	return result;
// }

// antlrcpp::Any AstBuilder::visitStmtBlock(PexlParser::StmtBlockContext* context) {
// 	return asBaseStatement(context->block()->accept(this).as<std::shared_ptr<Block>>());
// }

// antlrcpp::Any AstBuilder::visitStmtAtomic(PexlParser::StmtAtomicContext* /*context*/) {
// 	throw std::logic_error("not yet implemented (visitStmtAtomic)");
// }

// antlrcpp::Any AstBuilder::visitStmtWhile(PexlParser::StmtWhileContext* context) {
// 	auto result = makeBaseStatement<Loop>();
// 	result->body = context->block()->accept(this).as<std::shared_ptr<Block>>();
// 	return asBaseStatement(result);
// }

// antlrcpp::Any AstBuilder::visitStmtIfThenElse(PexlParser::StmtIfThenElseContext* context) {
// 	auto mkBranch = [&](PexlParser::BlockContext* blockContext, bool negateCondition) -> std::shared_ptr<Block> {
// 		auto result = blockContext->accept(this).as<std::shared_ptr<Block>>();
// 		auto condition = context->condition()->accept(this).as<std::shared_ptr<Condition>>();
// 		auto assume = makeAssumeFromCondition(condition, negateCondition);
// 		result->statements.insert(result->statements.begin(), assume);
// 		return result;
// 	};

// 	auto result = makeBaseStatement<Choice>();
// 	result->branches.push_back(mkBranch(context->ifBlock, false));
// 	if (context->elseBlock) {
// 		result->branches.push_back(mkBranch(context->elseBlock, true));
// 	}
// 	return asBaseStatement(result);
// }

// antlrcpp::Any AstBuilder::visitStmtIfCasElse(PexlParser::StmtIfCasElseContext* context) {
// 	auto cas = context->cas()->accept(this).as<std::shared_ptr<Choice>>();
// 	auto ifBranch = context->ifBlock->accept(this).as<std::shared_ptr<Block>>();
// 	auto elseBranch = context->elseBlock->accept(this).as<std::shared_ptr<Block>>();
	
// 	// TODO: this is not robust against changes! (it heavily relies on the internals of the CAS translation)
// 	auto addStmts = [&](std::shared_ptr<Block> toBlock, std::shared_ptr<Block> fromBlock) {
// 		auto& to = toBlock->statements;
// 		auto& from = fromBlock->statements;
// 		to.insert(to.end(), from.begin(), from.end());
// 	};
// 	addStmts(cas->branches.at(0), ifBranch);
// 	addStmts(cas->branches.at(1), elseBranch);
	
// 	return asBaseStatement(cas);
// }

// antlrcpp::Any AstBuilder::visitStmtReturnVarImm(PexlParser::StmtReturnVarImmContext* context) {
// 	auto varorval = context->value->accept(this).as<VarOrVal>();

// 	if (varorval.sort() == VarOrVal::VAL) {
// 		if (varorval.immediate() == Immediate::NULLPTR) {
// 			// TODO: proper error class
// 			throw std::logic_error("Returing NULL is not supported.");
// 		}

// 		auto result = makeBaseStatement<ReturnImmediateStatement>();
// 		result->immediate = varorval.immediate();
// 		return asBaseStatement(result);

// 	} else if (varorval.sort() == VarOrVal::VAL) {
// 		auto result = makeBaseStatement<ReturnVariableStatement>();
// 		result->variable = varorval.variable();
// 		return asBaseStatement(result);
// 	}

// 	// TODO: proper error class
// 	throw std::logic_error("Unsupported return.");
// }

// antlrcpp::Any AstBuilder::visitStmtReturnVoid(PexlParser::StmtReturnVoidContext* /*context*/) {
// 	return asBaseStatement(makeBaseStatement<ReturnVoidStatement>());
// }

// antlrcpp::Any AstBuilder::visitStmtAssume(PexlParser::StmtAssumeContext* context) {
// 	auto result = makeBaseStatement<AssumeStatement>();
// 	result->condition = context->condition()->accept(this).as<std::shared_ptr<Condition>>();
// 	return asBaseStatement(result);
// }

// antlrcpp::Any AstBuilder::visitStmtContinue(PexlParser::StmtContinueContext* /*context*/) {
// 	return asBaseStatement(makeBaseStatement<ContinueStatement>());
// }

// antlrcpp::Any AstBuilder::visitStmtBreak(PexlParser::StmtBreakContext* /*context*/) {
// 	return asBaseStatement(makeBaseStatement<BreakStatement>());
// }

// antlrcpp::Any AstBuilder::visitStmtSkip(PexlParser::StmtSkipContext* /*context*/) {
// 	return asBaseStatement(makeBaseStatement<Skip>());
// }

// antlrcpp::Any AstBuilder::visitStmtLinearizationPoint(PexlParser::StmtLinearizationPointContext* /*context*/) {
// 	// TODO: add skip command with linearization point
// 	throw std::logic_error("not yet implemented (visitStmtLinearizationPoint)");
// }

// antlrcpp::Any AstBuilder::visitStmtMalloc(PexlParser::StmtMallocContext* context) {
// 	auto result = makeBaseStatement<Malloc>();
// 	result->variable = lookupVariableOrFail(context->lhs->getText());
// 	return asBaseStatement(result);
// }

// antlrcpp::Any AstBuilder::visitStmtFree(PexlParser::StmtFreeContext* context) {
// 	auto result = makeBaseStatement<Free>();
// 	result->variable = lookupVariableOrFail(context->lhs->getText());
// 	return asBaseStatement(result);
// }

// antlrcpp::Any AstBuilder::visitStmtCas(PexlParser::StmtCasContext* /*context*/) {
// 	// TODO: this is painful -- we need to assign multiple variables here
// 	throw std::logic_error("not yet implemented (visitStmtCas)");
// }

// static void check_types(Type type1, Type type2) {
// 	if (type1 != type2) {
// 		// TODO: proper error class
// 		throw std::logic_error("Type missmatch.");
// 	}
// }
// static void check_types(std::shared_ptr<VariableDeclaration> decl, VarOrVal varorval) {
// 	check_types(decl->type, varorval.type());
// }
// static void check_types(std::shared_ptr<VariableDeclaration> decl1, std::shared_ptr<VariableDeclaration> decl2) {
// 	check_types(decl1->type, decl2->type);
// }

// static std::shared_ptr<Assignment> makeAssignmentFromVariableOrImmediate(AstBuilder& builder, std::shared_ptr<VariableDeclaration> lhsVar, Type lhsSel, VarOrVal rhs) {
// 	// lhsSel == Type::VOID indicates that the left hand side is a plain variable, not actually a selector

// 	Type typeLhs = lhsSel == Type::VOID ? lhsVar->type : lhsSel;
// 	check_types(typeLhs, rhs.type());

// 	if (lhsSel != Type::VOID) {
// 		// selectors may appear only on pointers
// 		check_types(lhsVar->type, Type::RAW_POINTER);
// 	}

// 	auto fromImm = [rhs](auto ass) -> auto { ass->rhs = rhs.immediate(); return ass; };
// 	auto fromVar = [rhs](auto ass) -> auto { ass->rhs = rhs.variable(); return ass; };
// 	auto toVar = [lhsVar](auto ass) -> auto { ass->lhs = lhsVar; return ass; };
// 	auto toSel = [lhsVar,lhsSel](auto ass) -> auto { ass->lhs_variable = lhsVar; ass->lhs_selector = lhsSel; return ass; };

// 	switch (rhs.sort()) {
// 		case VarOrVal::VAL:
// 			if (lhsSel == Type::VOID) {
// 				return toVar(fromImm((builder.makeBaseStatement<AssignmentToVariableFromImmediate>())));
// 			} else {
// 				return toSel(fromImm((builder.makeBaseStatement<AssignmentToSelectorFromImmediate>())));
// 			}

// 		case VarOrVal::VAR:
// 			if (lhsSel == Type::VOID) {
// 				return toVar(fromVar(builder.makeBaseStatement<AssignmentToVariableFromVariable>()));
// 			} else {
// 				return toSel(fromVar(builder.makeBaseStatement<AssignmentToSelectorFromVariable>()));
// 			}

// 		case VarOrVal::VARADDONE:
// 			if (lhsSel == Type::VOID) {
// 				return toVar(fromVar(builder.makeBaseStatement<TagAssignmentToVariableFromVariableAddOne>()));
// 			} else {
// 				return toSel(fromVar(builder.makeBaseStatement<TagAssignmentToSelectorFromVariableAddOne>()));
// 			}
// 	}
// }

// antlrcpp::Any AstBuilder::visitStmtAssignToVarFromVarImm(PexlParser::StmtAssignToVarFromVarImmContext* context) {
// 	auto lhs = lookupVariableOrFail(context->lhs->getText());
// 	auto rhs = context->rhs->accept(this).as<VarOrVal>();
// 	return asBaseStatement(makeAssignmentFromVariableOrImmediate(*this, lhs, Type::VOID, rhs));
// }

// antlrcpp::Any AstBuilder::visitStmtAssignToSelFromVarImm(PexlParser::StmtAssignToSelFromVarImmContext* context) {
// 	auto lhsVar = lookupVariableOrFail(context->lhs->getText());
// 	auto lhsSel = context->selector->accept(this).as<Type>();
// 	auto rhs = context->rhs->accept(this).as<VarOrVal>();
// 	return asBaseStatement(makeAssignmentFromVariableOrImmediate(*this, lhsVar, lhsSel, rhs));
// }

// antlrcpp::Any AstBuilder::visitStmtAssignToVarFromSel(PexlParser::StmtAssignToVarFromSelContext* context) {
// 	auto lhs = lookupVariableOrFail(context->lhs->getText());
// 	auto rhsVar = lookupVariableOrFail(context->rhs->getText());
// 	auto rhsSel = context->selector->accept(this).as<Type>();
// 	check_types(lhs, rhsVar);

// 	auto result = makeBaseStatement<AssignmentToVariableFromSelector>();
// 	result->lhs = lhs;
// 	result->rhs_variable = rhsVar;
// 	result->rhs_selector = rhsSel;
// 	return asBaseStatement(result);
// }

// antlrcpp::Any AstBuilder::visitStmtAssignToVarFromHavoc(PexlParser::StmtAssignToVarFromHavocContext* context) {
// 	auto lhs = lookupVariableOrFail(context->lhs->getText());
// 	check_types(lhs->type, Type::DATA);
// 	auto result = makeBaseStatement<AssignmentToVariableFromImmediate>();
// 	result->lhs = lhs;
// 	result->rhs = Immediate::HAVOC;
// 	return asBaseStatement(result);
// }

// template<typename T>
// static void check_unique(const std::vector<T>& list) {
// 	for (std::size_t index = 0; index < list.size(); index++) {
// 		for (std::size_t other = index + 1; other < list.size(); index++) {
// 			if (list.at(index) == list.at(other)) {
// 				// TODO: proper error class
// 				throw std::logic_error("Element must not appear multiple times in list.");
// 			}
// 		}
// 	}
// }

// template<typename T>
// static void check_nonempty(const std::vector<T>& list) {
// 	if (list.size() == 0) {
// 		// TODO: proper error class
// 		throw std::logic_error("Empty list detected. Treated as error.");
// 	}
// }

// template<typename T, typename S>
// static void check_balanced(const std::vector<T>& list1, const std::vector<S>& list2) {
// 	if (list1.size() != list2.size()) {
// 		// TODO: proper error class
// 		throw std::logic_error("Imbalanced lists.");
// 	}
// }

// antlrcpp::Any AstBuilder::visitStmtAssignMultipleToVarFromVar(PexlParser::StmtAssignMultipleToVarFromVarContext* context) {
// 	auto lhs = context->lhs->accept(this).as<std::vector<std::shared_ptr<VariableDeclaration>>>();
// 	auto rhs = context->rhs->accept(this).as<std::vector<std::shared_ptr<VariableDeclaration>>>();
	
// 	check_unique(lhs);
// 	check_nonempty(lhs);
// 	check_balanced(lhs, rhs);

// 	std::vector<std::shared_ptr<Statement>> stmts;
// 	for (std::size_t index = 0; index < lhs.size(); index++) {
// 		auto lhsDecl = lhs.at(index);
// 		auto rhsDecl = rhs.at(index);
// 		auto assignment = makeAssignmentFromVariableOrImmediate(*this, lhsDecl, Type::VOID, rhsDecl);
// 		stmts.push_back(assignment);
// 	}

// 	auto result = makeBaseStatement<AtomicBlock>();
// 	result->statements = stmts;
// 	return asBaseStatement(result);
// }

// antlrcpp::Any AstBuilder::visitStmtAssignMultipleToVarFromSel(PexlParser::StmtAssignMultipleToVarFromSelContext* context) {
// 	auto lhs = context->lhs->accept(this).as<std::vector<std::shared_ptr<VariableDeclaration>>>();
// 	auto rhs = lookupVariableOrFail(context->rhs->getText());
// 	auto selector = context->selector->accept(this).as<std::vector<Type>>();
	
// 	check_unique(lhs);
// 	check_nonempty(lhs);
// 	check_balanced(lhs, selector);

// 	std::vector<std::shared_ptr<Statement>> stmts;
// 	for (std::size_t index = 0; index < lhs.size(); index++) {
// 		auto lhsDecl = lhs.at(index);
// 		auto rhsDecl = rhs;
// 		auto rhsSel = selector.at(index);
// 		check_types(lhsDecl->type, rhsSel);
		
// 		auto assignment = makeBaseStatement<AssignmentToVariableFromSelector>();
// 		assignment->lhs = lhsDecl;
// 		assignment->rhs_variable = rhsDecl;
// 		assignment->rhs_selector = rhsSel;
		
// 		stmts.push_back(assignment);
// 	}

// 	auto result = makeBaseStatement<AtomicBlock>();
// 	result->statements = stmts;
// 	return asBaseStatement(result);
// }

// template<typename T>
// std::shared_ptr<T> AstBuilder::makeBaseCondition() {
// 	auto result = std::make_shared<T>();
// 	return result;
// }

// static void check_condition_immediate(Immediate immi) {
// 	switch (immi) {
// 		case Immediate::NULLPTR:
// 		case Immediate::TRUE:
// 		case Immediate::FALSE:
// 			return;

// 		case Immediate::EMPTY:
// 		case Immediate::HAVOC:
// 			throw std::logic_error("Disallowed immedidate value for comparision.");
// 	}
// }

// static bool check_comparison_operator(Type type, ComparisonOperator op) {
// 	switch (op) {
// 		case ComparisonOperator::LEQ:
// 		case ComparisonOperator::LT:
// 		case ComparisonOperator::GEQ:
// 		case ComparisonOperator::GT:
// 			if (type == Type::DATA) return true;
// 			else return false;

// 		case ComparisonOperator::EQ:
// 		case ComparisonOperator::NEQ:
// 			if (type != Type::VOID) return false;
// 			else return true;
// 	}
// }

// static std::shared_ptr<ComparisonCondition> makeComparisonCondition(AstBuilder& builder, std::shared_ptr<VariableDeclaration> lhsVar, Type lhsSel, VarOrVal rhs, ComparisonOperator comparator) {
// 	auto lhsType = lhsSel == Type::VOID ? lhsVar->type : lhsSel;
// 	check_types(lhsType, rhs.type());
// 	check_comparison_operator(lhsType, comparator);
// 	std::shared_ptr<ComparisonCondition> result;

// 	switch (rhs.sort()) {
// 		case VarOrVal::VARADDONE:
// 			throw std::logic_error("Arithmetic expression not allowed in comparison.");

// 		case VarOrVal::VAR: {
// 			if (lhsSel == Type::VOID) {
// 				auto tmp = builder.makeBaseCondition<ComparisonConditionVariableAndVariabel>();
// 				tmp->lhs = lhsVar;
// 				tmp->rhs = rhs.variable();
// 				result = tmp;
// 			} else {
// 				auto tmp = builder.makeBaseCondition<ComparisonConditionSelectorAndVariabel>();
// 				tmp->lhs_variable = lhsVar;
// 				tmp->lhs_selector = lhsSel;
// 				tmp->rhs = rhs.variable();
// 				result = tmp;
// 			}
// 			break;
// 		}

// 		case VarOrVal::VAL: {
// 			std::shared_ptr<ComparisonConditionRhsImmediate> result;
// 			check_condition_immediate(rhs.immediate());
// 			if (lhsSel == Type::VOID) {
// 				auto tmp = builder.makeBaseCondition<ComparisonConditionVariableAndImmediate>();
// 				tmp->lhs = lhsVar;
// 				tmp->rhs = rhs.immediate();
// 				result = tmp;
// 			} else {
// 				auto tmp = builder.makeBaseCondition<ComparisonConditionSelectorAndImmediate>();
// 				tmp->lhs_variable = lhsVar;
// 				tmp->lhs_selector = lhsSel;
// 				tmp->rhs = rhs.immediate();
// 				result = tmp;
// 			}
// 			break;
// 		}
// 	}

// 	result->comparison_op = comparator;
// 	return result;
// }

// static std::shared_ptr<CompoundCondition> makeCompoundCondition(AstBuilder& builder, std::vector<std::shared_ptr<ComparisonCondition>> conditions, LogicOperator combiner) {
// 	check_nonempty(conditions);
// 	auto result = builder.makeBaseCondition<CompoundCondition>();
// 	result->logic_op = combiner;
// 	result->conditions = conditions;
// 	return result;
// }

// static std::shared_ptr<Choice> makeCompareAndSwap(AstBuilder& builder, std::vector<std::shared_ptr<VariableDeclaration>> dstVar, std::vector<Type> dstSel, std::vector<VarOrVal> cmp, std::vector<VarOrVal> src, std::shared_ptr<LinearizationPoint> linp) {
// 	check_nonempty(dstVar);
// 	check_balanced(dstVar, dstSel);
// 	check_balanced(dstVar, cmp);
// 	check_balanced(cmp, src);

// 	std::vector<std::shared_ptr<ComparisonCondition>> conditionsEq;
// 	std::vector<std::shared_ptr<ComparisonCondition>> conditionsNeq;
// 	std::vector<std::shared_ptr<Statement>> statements;
// 	conditionsEq.reserve(dstVar.size());
// 	conditionsNeq.reserve(dstVar.size());
// 	statements.reserve(dstVar.size() + 1);

// 	for (std::size_t index = 0; index < dstVar.size(); index++) {
// 		auto dstDecl = dstVar.at(index);
// 		auto dstSeli = dstSel.at(index);
// 		auto cmpImmi = cmp.at(index);
// 		auto srcImmi = src.at(index);

// 		// type check is done at call site
// 		conditionsEq.push_back(makeComparisonCondition(builder, dstDecl, dstSeli, cmpImmi, ComparisonOperator::EQ));
// 		conditionsNeq.push_back(makeComparisonCondition(builder, dstDecl, dstSeli, cmpImmi, ComparisonOperator::NEQ));
// 		statements.push_back(makeAssignmentFromVariableOrImmediate(builder, dstDecl, dstSeli, srcImmi));
// 	}

// 	auto swapCondition = makeCompoundCondition(builder, conditionsEq, LogicOperator::AND);
// 	auto skipCondition = makeCompoundCondition(builder, conditionsNeq, LogicOperator::OR);

// 	auto swapAssume = builder.makeBaseStatement<AssumeStatement>();
// 	auto skipAssume = builder.makeBaseStatement<AssumeStatement>();
// 	swapAssume->condition = swapCondition;
// 	skipAssume->condition = skipCondition;

// 	if (linp) {
// 		swapAssume->linearization_point = linp;
// 	}

// 	statements.insert(statements.begin(), swapAssume);
// 	auto swapBlock = builder.makeBaseStatement<AtomicBlock>();
// 	swapBlock->statements = statements;

// 	auto result = builder.makeBaseStatement<Choice>();
// 	auto branchSwap = builder.makeBaseStatement<Block>();
// 	auto branchSkip = builder.makeBaseStatement<Block>();

// 	branchSwap->statements.push_back(swapBlock);
// 	branchSkip->statements.push_back(skipAssume);

// 	result->branches.push_back(branchSwap);
// 	result->branches.push_back(branchSkip);

// 	return result;
// }

// antlrcpp::Any AstBuilder::visitCasMultipleVar(PexlParser::CasMultipleVarContext* context) {
// 	auto dst = context->dst->accept(this).as<std::vector<std::shared_ptr<VariableDeclaration>>>();
// 	auto cmp = context->cmp->accept(this).as<std::vector<VarOrVal>>();
// 	auto src = context->src->accept(this).as<std::vector<VarOrVal>>();
// 	auto linp = context->linp() ? context->linp()->accept(this).as<std::shared_ptr<LinearizationPoint>>() : nullptr;
// 	auto dummyDstSel = std::vector<Type>(dst.size(), Type::VOID);

// 	return makeCompareAndSwap(*this, dst, dummyDstSel, cmp, src, linp);
// }
// antlrcpp::Any AstBuilder::visitCasMultipleSel(PexlParser::CasMultipleSelContext* context) {
// 	auto dstVar = lookupVariableOrFail(context->dst->getText());
// 	auto dstSel = context->selector->accept(this).as<std::vector<Type>>();
// 	auto cmp = context->cmp->accept(this).as<std::vector<VarOrVal>>();
// 	auto src = context->src->accept(this).as<std::vector<VarOrVal>>();
// 	auto linp = context->linp() ? context->linp()->accept(this).as<std::shared_ptr<LinearizationPoint>>() : nullptr;
// 	auto dummyVarList = std::vector<std::shared_ptr<VariableDeclaration>>(dstSel.size(), dstVar);

// 	return makeCompareAndSwap(*this, dummyVarList, dstSel, cmp, src, linp);
// }

// antlrcpp::Any AstBuilder::visitIdentList(PexlParser::IdentListContext* context) {
// 	std::vector<std::shared_ptr<VariableDeclaration>> result;
// 	for (auto name : context->ident) {
// 		auto declaration = lookupVariableOrFail(name->getText());
// 		result.push_back(declaration);
// 	}
// 	return result;
// }

// antlrcpp::Any AstBuilder::visitFieldList(PexlParser::FieldListContext* context) {
// 	// returns a list of types for the corresponding field
// 	std::vector<Type> result;
// 	for (auto field : context->field) {
// 		result.push_back(field->accept(this));
// 	}
// 	return result;
// }

// antlrcpp::Any AstBuilder::visitCasList(PexlParser::CasListContext* context) {
// 	std::vector<VarOrVal> result;
// 	for (auto obj : context->expr) {
// 		result.push_back(obj->accept(this));
// 	}
// 	return result;
// }

// antlrcpp::Any AstBuilder::visitVarimmVar(PexlParser::VarimmVarContext* context) {
// 	auto variableDeclaration = lookupVariableOrFail(context->ident->getText());
// 	return VarOrVal(variableDeclaration);
// }

// antlrcpp::Any AstBuilder::visitVarimmVarPlusOne(PexlParser::VarimmVarPlusOneContext* context) {
// 	auto variableDeclaration = lookupVariableOrFail(context->ident->getText());
// 	if (variableDeclaration->type != Type::TAG) {
// 		// TODO: proper error class
// 		throw std::logic_error("Cannot add 1 to non-tag type.");
// 	}
// 	return VarOrVal(variableDeclaration, true);
// }

// antlrcpp::Any AstBuilder::visitVarimmImm(PexlParser::VarimmImmContext* context) {
// 	return context->immediate()->accept(this);
// }

// antlrcpp::Any AstBuilder::visitImmediateTrue(PexlParser::ImmediateTrueContext* /*context*/) {
// 	return VarOrVal(Immediate::TRUE);
// }

// antlrcpp::Any AstBuilder::visitImmediateFalse(PexlParser::ImmediateFalseContext* /*context*/) {
// 	return VarOrVal(Immediate::FALSE);
// }

// antlrcpp::Any AstBuilder::visitImmediateNull(PexlParser::ImmediateNullContext* /*context*/) {
// 	return VarOrVal(Immediate::NULLPTR);
// }

// antlrcpp::Any AstBuilder::visitImmediateEmpty(PexlParser::ImmediateEmptyContext* /*context*/) {
// 	return VarOrVal(Immediate::EMPTY);
// }

// antlrcpp::Any AstBuilder::visitFieldPointer(PexlParser::FieldPointerContext* /*context*/) {
// 	return Type::RAW_POINTER;
// }

// antlrcpp::Any AstBuilder::visitFieldDate(PexlParser::FieldDateContext* /*context*/) {
// 	return Type::DATA;
// }

// antlrcpp::Any AstBuilder::visitFieldTag(PexlParser::FieldTagContext* /*context*/) {
// 	return Type::TAG;
// }

// antlrcpp::Any AstBuilder::visitFieldMark(PexlParser::FieldMarkContext* /*context*/) {
// 	return Type::BOOL;
// }


// antlrcpp::Any AstBuilder::visitConditionComparison(PexlParser::ConditionComparisonContext* context) {
// 	auto cmpcond = context->cmp->accept(this).as<std::shared_ptr<ComparisonCondition>>();
// 	return makeCompoundCondition(*this, {{ cmpcond }}, LogicOperator::AND);
// }

// antlrcpp::Any AstBuilder::visitConditionConjunction(PexlParser::ConditionConjunctionContext* context) {
// 	std::vector<std::shared_ptr<ComparisonCondition>> comparisons;
// 	comparisons.reserve(context->cmp.size());
// 	for (auto comp : context->cmp) {
// 		comparisons.push_back(comp->accept(this).as<std::shared_ptr<ComparisonCondition>>());
// 	}
// 	return makeCompoundCondition(*this, comparisons, LogicOperator::AND);
// }

// antlrcpp::Any AstBuilder::visitConditionDisjunction(PexlParser::ConditionDisjunctionContext* context) {
// 	std::vector<std::shared_ptr<ComparisonCondition>> comparisons;
// 	comparisons.reserve(context->cmp.size());
// 	for (auto comp : context->cmp) {
// 		comparisons.push_back(comp->accept(this).as<std::shared_ptr<ComparisonCondition>>());
// 	}
// 	return makeCompoundCondition(*this, comparisons, LogicOperator::AND);
// }

// antlrcpp::Any AstBuilder::visitConparisonVar(PexlParser::ConparisonVarContext* context) {
// 	auto variable = lookupVariableOrFail(context->var->getText());
// 	if (variable->type == Type::BOOL) {
// 		return makeComparisonCondition(*this, variable, Type::VOID, VarOrVal(Immediate::TRUE), ComparisonOperator::EQ);
// 	} else {
// 		// TODO: proper error class
// 		throw std::logic_error("Auto cast of non-boolean variable to boolean not supported.");
// 	}
// }

// antlrcpp::Any AstBuilder::visitConparisonVarImmVarImm(PexlParser::ConparisonVarImmVarImmContext* context) {
// 	auto lhs = context->lhs->accept(this).as<VarOrVal>();
// 	auto rhs = context->rhs->accept(this).as<VarOrVal>();
// 	auto op = context->compop()->accept(this).as<ComparisonOperator>();

// 	if (lhs.sort() == VarOrVal::VARADDONE || rhs.sort() == VarOrVal::VARADDONE) {
// 		// TODO: proper error class
// 		throw std::logic_error("Arithmetic not supported in comparisons.");
// 	}

// 	if (lhs.sort() == rhs.sort() && lhs.sort() == VarOrVal::VAL) {
// 		// TODO: proper error class
// 		throw std::logic_error("Comparing two immediate values -- considered as bug.");
// 	}

// 	if (lhs.sort() == VarOrVal::VAR) {
// 		return makeComparisonCondition(*this, lhs.variable(), Type::VOID, rhs, op);

// 	} else if (rhs.sort() == VarOrVal::VAR) {
// 		return makeComparisonCondition(*this, rhs.variable(), Type::VOID, lhs, op);

// 	} else {
// 		// TODO: proper error class
// 		throw std::logic_error("Translation unsuccessful");
// 	}
// }

// antlrcpp::Any AstBuilder::visitConparisonSelVarImm(PexlParser::ConparisonSelVarImmContext* context) {
// 	auto lhsVariabel = lookupVariableOrFail(context->lhs->getText());
// 	auto lhsSelector = context->selector->accept(this).as<Type>();
// 	auto rhs = context->rhs->accept(this).as<VarOrVal>();
// 	auto op = context->compop()->accept(this).as<ComparisonOperator>();
// 	return makeComparisonCondition(*this, lhsVariabel, lhsSelector, rhs, op);
// }

// antlrcpp::Any AstBuilder::visitConparisonVarImmSel(PexlParser::ConparisonVarImmSelContext* context) {
// 	auto lhs = context->lhs->accept(this).as<VarOrVal>();
// 	auto rhsVariabel = lookupVariableOrFail(context->rhs->getText());
// 	auto rhsSelector = context->selector->accept(this).as<Type>();
// 	auto op = context->compop()->accept(this).as<ComparisonOperator>();
// 	return makeComparisonCondition(*this, rhsVariabel, rhsSelector, lhs, op);
// }

// antlrcpp::Any AstBuilder::visitCmpopEq(PexlParser::CmpopEqContext* /*context*/) {
// 	return ComparisonOperator::EQ;
// }

// antlrcpp::Any AstBuilder::visitCmpopNeq(PexlParser::CmpopNeqContext* /*context*/) {
// 	return ComparisonOperator::NEQ;
// }

// antlrcpp::Any AstBuilder::visitCmpopLt(PexlParser::CmpopLtContext* /*context*/) {
// 	return ComparisonOperator::LT;
// }

// antlrcpp::Any AstBuilder::visitCmpopLte(PexlParser::CmpopLteContext* /*context*/) {
// 	return ComparisonOperator::LEQ;
// }

// antlrcpp::Any AstBuilder::visitCmpopGt(PexlParser::CmpopGtContext* /*context*/) {
// 	return ComparisonOperator::GT;
// }

// antlrcpp::Any AstBuilder::visitCmpopGte(PexlParser::CmpopGteContext* /*context*/) {
// 	return ComparisonOperator::GEQ;
// }


// antlrcpp::Any AstBuilder::visitLinp(antlrcpptest::PexlParser::LinpContext* context) {
// 	auto cond = context->cond->accept(this).as<std::shared_ptr<Condition>>();
// 	auto func = context->func->getText();
// 	auto param = context->param->accept(this).as<VarOrVal>();

// 	// TODO: support out-of-line linearization points
// 	if (func != _currentFunction->name) {
// 		// TODO: proper error class
// 		throw std::logic_error("Linearization point must not occur outside method.");
// 	}

// 	std::shared_ptr<LinearizationPoint> result;

// 	if (param.sort() == VarOrVal::VAR) {
// 		auto tmp = std::make_shared<LinearizationPointFromDataSelector>();
// 		tmp->parameter = param.variable();
// 		result = tmp;

// 	} else if (param.sort() == VarOrVal::VAL) {
// 		auto tmp = std::make_shared<LinearizationPointFromImmediate>();
// 		tmp->parameter = param.immediate();
// 		result = tmp;

// 	} else {
// 		// TODO: proper error class
// 		throw std::logic_error("Linearization with unsupported parameter.");
// 	}

// 	result->condition = cond;
// 	result->function = _currentFunction;
// 	return result;
// }

// // TODO: asBaseStatement-Construction for expressions ??

