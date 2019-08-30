#pragma once

#include <memory>
#include <deque>
#include <unordered_map>
// TODO: import string
#include "antlr4-runtime.h"
#include "CoLaVisitor.h"
#include "cola/ast.hpp"


namespace cola {

	class AstBuilder : public cola::CoLaVisitor { // TODO: should this be a private subclass to avoid misuse?
		private:
			const std::string INIT_NAME = "init";
			enum struct Modifier { NONE, INLINE, EXTERN };
			enum struct ExprForm { NOPTR, PTR, DEREF, NESTED };
			struct ExprTrans {
				ExprForm form;
				std::vector<Statement*> helper;
				Expression* expr;
				std::size_t num_shared;
				ExprTrans() {}
				ExprTrans(ExprForm form, Expression* expr) : form(form), expr(expr), num_shared(0) {}
				ExprTrans(ExprForm form, Expression* expr, std::size_t num_shared) : form(form), expr(expr), num_shared(num_shared) {}
			};
			using TypeMap = std::unordered_map<std::string, std::reference_wrapper<const Type>>;
			using VariableMap = std::unordered_map<std::string, std::unique_ptr<VariableDeclaration>>;
			using FunctionMap = std::unordered_map<std::string, Function&>;
			using ArgDeclList = std::vector<std::pair<std::string, std::string>>;
			std::shared_ptr<Program> _program = nullptr;
			std::deque<VariableMap> _scope;
			TypeMap _types;
			FunctionMap _functions;
			bool _inside_loop = false;
			std::unique_ptr<Invariant> _cmdInvariant;
			const Function* _currentFunction;

			void pushScope();
			std::vector<std::unique_ptr<VariableDeclaration>> popScope();
			void addVariable(std::unique_ptr<VariableDeclaration> variable);
			bool isVariableDeclared(std::string variableName);
			const VariableDeclaration& lookupVariable(std::string variableName);
			bool isTypeDeclared(std::string typeName);
			const Type& lookupType(std::string typeName);
			std::unique_ptr<Statement> mk_stmt_from_list(std::vector<cola::CoLaParser::StatementContext*> stmts);
			std::unique_ptr<Statement> mk_stmt_from_list(std::vector<Statement*> stmts); // claims ownership of stmts
			Statement* as_command(Statement* stmt, AnnotatedStatement* cmd);
			Statement* as_command(AnnotatedStatement* stmt) { return as_command(stmt, stmt); }
			

		public:
			static std::shared_ptr<Program> buildFrom(cola::CoLaParser::ProgramContext* parseTree);

			antlrcpp::Any visitProgram(cola::CoLaParser::ProgramContext* context) override;
			antlrcpp::Any visitStruct_decl(cola::CoLaParser::Struct_declContext* context) override;
			antlrcpp::Any visitNameVoid(cola::CoLaParser::NameVoidContext* context) override;
			antlrcpp::Any visitNameBool(cola::CoLaParser::NameBoolContext* context) override;
			antlrcpp::Any visitNameInt(cola::CoLaParser::NameIntContext* context) override;
			antlrcpp::Any visitNameData(cola::CoLaParser::NameDataContext* context) override;
			antlrcpp::Any visitNameIdentifier(cola::CoLaParser::NameIdentifierContext* context) override;
			antlrcpp::Any visitTypeValue(cola::CoLaParser::TypeValueContext* context) override;
			antlrcpp::Any visitTypePointer(cola::CoLaParser::TypePointerContext* context) override;
			antlrcpp::Any visitField_decl(cola::CoLaParser::Field_declContext* context) override;
			antlrcpp::Any visitVar_decl(cola::CoLaParser::Var_declContext* context) override;
			antlrcpp::Any visitFunction(cola::CoLaParser::FunctionContext* context) override;
			antlrcpp::Any visitArgDeclList(cola::CoLaParser::ArgDeclListContext *context) override;

			antlrcpp::Any visitBlockStmt(cola::CoLaParser::BlockStmtContext* context) override;
			antlrcpp::Any visitBlockScope(cola::CoLaParser::BlockScopeContext* context) override;
	    	antlrcpp::Any visitScope(cola::CoLaParser::ScopeContext* context) override;
			antlrcpp::Any visitStmtIf(cola::CoLaParser::StmtIfContext* context) override;
			antlrcpp::Any visitStmtWhile(cola::CoLaParser::StmtWhileContext* context) override;
			antlrcpp::Any visitStmtDo(cola::CoLaParser::StmtDoContext* context) override;
			antlrcpp::Any visitStmtChoose(cola::CoLaParser::StmtChooseContext* context) override;
			antlrcpp::Any visitStmtLoop(cola::CoLaParser::StmtLoopContext* context) override;
			antlrcpp::Any visitStmtAtomic(cola::CoLaParser::StmtAtomicContext* context) override;
			antlrcpp::Any visitStmtCom(cola::CoLaParser::StmtComContext* context) override;
			antlrcpp::Any visitAnnotation(cola::CoLaParser::AnnotationContext* context) override;

			antlrcpp::Any visitCmdSkip(cola::CoLaParser::CmdSkipContext* context) override;
			antlrcpp::Any visitCmdAssign(cola::CoLaParser::CmdAssignContext* context) override;
			antlrcpp::Any visitCmdMalloc(cola::CoLaParser::CmdMallocContext* context) override;
			antlrcpp::Any visitCmdAssume(cola::CoLaParser::CmdAssumeContext* context) override;
			antlrcpp::Any visitCmdAssert(cola::CoLaParser::CmdAssertContext* context) override;
			antlrcpp::Any visitCmdAngel(cola::CoLaParser::CmdAngelContext* context) override;
			antlrcpp::Any visitCmdCall(cola::CoLaParser::CmdCallContext* context) override;
			antlrcpp::Any visitCmdContinue(cola::CoLaParser::CmdContinueContext* context) override;
			antlrcpp::Any visitCmdBreak(cola::CoLaParser::CmdBreakContext* context) override;
			antlrcpp::Any visitCmdReturn(cola::CoLaParser::CmdReturnContext* context) override;
			antlrcpp::Any visitCmdCas(cola::CoLaParser::CmdCasContext* context) override;
			antlrcpp::Any visitArgList(cola::CoLaParser::ArgListContext* context) override;
			antlrcpp::Any visitCas(cola::CoLaParser::CasContext* context) override;

			antlrcpp::Any visitOpEq(cola::CoLaParser::OpEqContext* context) override;
			antlrcpp::Any visitOpNeq(cola::CoLaParser::OpNeqContext* context) override;
			antlrcpp::Any visitOpLt(cola::CoLaParser::OpLtContext* context) override;
			antlrcpp::Any visitOpLte(cola::CoLaParser::OpLteContext* context) override;
			antlrcpp::Any visitOpGt(cola::CoLaParser::OpGtContext* context) override;
			antlrcpp::Any visitOpGte(cola::CoLaParser::OpGteContext* context) override;
			antlrcpp::Any visitOpAnd(cola::CoLaParser::OpAndContext* context) override;
			antlrcpp::Any visitOpOr(cola::CoLaParser::OpOrContext* context) override;
			antlrcpp::Any visitValueNull(cola::CoLaParser::ValueNullContext* context) override;
			antlrcpp::Any visitValueTrue(cola::CoLaParser::ValueTrueContext* context) override;
			antlrcpp::Any visitValueFalse(cola::CoLaParser::ValueFalseContext* context) override;
			antlrcpp::Any visitValueNDet(cola::CoLaParser::ValueNDetContext* context) override;
			antlrcpp::Any visitValueEmpty(cola::CoLaParser::ValueEmptyContext* context) override;
			antlrcpp::Any visitValueMin(cola::CoLaParser::ValueMinContext* context) override;
			antlrcpp::Any visitValueMax(cola::CoLaParser::ValueMaxContext* context) override;
			antlrcpp::Any visitExprValue(cola::CoLaParser::ExprValueContext* context) override;
			antlrcpp::Any visitExprBinary(cola::CoLaParser::ExprBinaryContext* context) override;
			antlrcpp::Any visitExprIdentifier(cola::CoLaParser::ExprIdentifierContext* context) override;
			antlrcpp::Any visitExprParens(cola::CoLaParser::ExprParensContext* context) override;
			antlrcpp::Any visitExprDeref(cola::CoLaParser::ExprDerefContext* context) override;
	    	antlrcpp::Any visitExprCas(cola::CoLaParser::ExprCasContext* context) override;
			antlrcpp::Any visitExprNegation(cola::CoLaParser::ExprNegationContext* context) override;
			antlrcpp::Any visitInvExpr(cola::CoLaParser::InvExprContext* context) override;
			antlrcpp::Any visitInvActive(cola::CoLaParser::InvActiveContext* context) override;
			antlrcpp::Any visitAngelChoose(cola::CoLaParser::AngelChooseContext* context) override;
			antlrcpp::Any visitAngelActive(cola::CoLaParser::AngelActiveContext* context) override;
			antlrcpp::Any visitAngelChooseActive(cola::CoLaParser::AngelChooseActiveContext* context) override;
			antlrcpp::Any visitAngelContains(cola::CoLaParser::AngelContainsContext* context) override;

			antlrcpp::Any visitOption(cola::CoLaParser::OptionContext* context) override;


			antlrcpp::Any visitObserverList(cola::CoLaParser::ObserverListContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitObserverDefinition(cola::CoLaParser::ObserverDefinitionContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitObserverVariableList(cola::CoLaParser::ObserverVariableListContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitObserverVariable(cola::CoLaParser::ObserverVariableContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitObserverStateList(cola::CoLaParser::ObserverStateListContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitObserverState(cola::CoLaParser::ObserverStateContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitObserverTransitionList(cola::CoLaParser::ObserverTransitionListContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitObserverTransition(cola::CoLaParser::ObserverTransitionContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitObserverGuardTrue(cola::CoLaParser::ObserverGuardTrueContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitObserverGuardIdentifierEq(cola::CoLaParser::ObserverGuardIdentifierEqContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitObserverGuardIdentifierNeq(cola::CoLaParser::ObserverGuardIdentifierNeqContext* /*context*/) override { throw std::logic_error("not implemented"); }
	};

} // namespace cola
