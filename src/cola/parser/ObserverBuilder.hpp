#pragma once

#include <memory>
#include <deque>
#include <unordered_map>
// TODO: import string
#include "antlr4-runtime.h"
#include "CoLaVisitor.h"
#include "cola/ast.hpp"
#include "cola/observer.hpp"


namespace cola {

	class ObserverBuilder : public cola::CoLaVisitor { // TODO: should this be a private subclass to avoid misuse?
		private:
			std::vector<std::unique_ptr<Observer>> _observer_list;
			Observer* _observer = nullptr;
			std::unordered_map<std::string, State&> _name2state;
			std::unordered_map<std::string, ThreadObserverVariable&> _name2threadvar;
			std::unordered_map<std::string, ProgramObserverVariable&> _name2ptrvar;
			std::unordered_map<std::string, const Function*> _name2function;
			const VariableDeclaration* _current_arg = nullptr;

			void check_name_clash(std::string name);
			const State& find_state(std::string name);
			const Function& find_function(std::string name);
			const ThreadObserverVariable& find_threadvar(std::string name);
			const ProgramObserverVariable& find_ptrvar(std::string name);
			

		public:
			static std::vector<std::unique_ptr<Observer>> buildFrom(cola::CoLaParser::ObserverContext* parseTree, const Program& program);

			ObserverBuilder(const Program& program);

			antlrcpp::Any visitObserverList(cola::CoLaParser::ObserverListContext* context) override;
			antlrcpp::Any visitObserverDefinition(cola::CoLaParser::ObserverDefinitionContext* context) override;
			antlrcpp::Any visitObserverVariableList(cola::CoLaParser::ObserverVariableListContext* context) override;
			antlrcpp::Any visitObserverVariable(cola::CoLaParser::ObserverVariableContext* context) override;
			antlrcpp::Any visitObserverStateList(cola::CoLaParser::ObserverStateListContext* context) override;
			antlrcpp::Any visitObserverState(cola::CoLaParser::ObserverStateContext* context) override;
			antlrcpp::Any visitObserverTransitionList(cola::CoLaParser::ObserverTransitionListContext* context) override;
			antlrcpp::Any visitObserverTransition(cola::CoLaParser::ObserverTransitionContext* context) override;
			antlrcpp::Any visitObserverGuardTrue(cola::CoLaParser::ObserverGuardTrueContext* context) override;
			antlrcpp::Any visitObserverGuardIdentifierEq(cola::CoLaParser::ObserverGuardIdentifierEqContext* context) override;
			antlrcpp::Any visitObserverGuardIdentifierNeq(cola::CoLaParser::ObserverGuardIdentifierNeqContext* context) override;

			antlrcpp::Any visitProgram(cola::CoLaParser::ProgramContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitStruct_decl(cola::CoLaParser::Struct_declContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitNameVoid(cola::CoLaParser::NameVoidContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitNameBool(cola::CoLaParser::NameBoolContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitNameInt(cola::CoLaParser::NameIntContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitNameData(cola::CoLaParser::NameDataContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitNameIdentifier(cola::CoLaParser::NameIdentifierContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitTypeValue(cola::CoLaParser::TypeValueContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitTypePointer(cola::CoLaParser::TypePointerContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitField_decl(cola::CoLaParser::Field_declContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitVar_decl(cola::CoLaParser::Var_declContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitFunction(cola::CoLaParser::FunctionContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitArgDeclList(cola::CoLaParser::ArgDeclListContext* /*context*/) override { throw std::logic_error("not implemented"); }

			antlrcpp::Any visitBlockStmt(cola::CoLaParser::BlockStmtContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitBlockScope(cola::CoLaParser::BlockScopeContext* /*context*/) override { throw std::logic_error("not implemented"); }
	    	antlrcpp::Any visitScope(cola::CoLaParser::ScopeContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitStmtIf(cola::CoLaParser::StmtIfContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitStmtWhile(cola::CoLaParser::StmtWhileContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitStmtDo(cola::CoLaParser::StmtDoContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitStmtChoose(cola::CoLaParser::StmtChooseContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitStmtLoop(cola::CoLaParser::StmtLoopContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitStmtAtomic(cola::CoLaParser::StmtAtomicContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitStmtCom(cola::CoLaParser::StmtComContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitAnnotation(cola::CoLaParser::AnnotationContext* /*context*/) override { throw std::logic_error("not implemented"); }

			antlrcpp::Any visitCmdSkip(cola::CoLaParser::CmdSkipContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitCmdAssign(cola::CoLaParser::CmdAssignContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitCmdMalloc(cola::CoLaParser::CmdMallocContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitCmdAssume(cola::CoLaParser::CmdAssumeContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitCmdAssert(cola::CoLaParser::CmdAssertContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitCmdAngel(cola::CoLaParser::CmdAngelContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitCmdCall(cola::CoLaParser::CmdCallContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitCmdContinue(cola::CoLaParser::CmdContinueContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitCmdBreak(cola::CoLaParser::CmdBreakContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitCmdReturn(cola::CoLaParser::CmdReturnContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitCmdCas(cola::CoLaParser::CmdCasContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitArgList(cola::CoLaParser::ArgListContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitCas(cola::CoLaParser::CasContext* /*context*/) override { throw std::logic_error("not implemented"); }

			antlrcpp::Any visitOpEq(cola::CoLaParser::OpEqContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitOpNeq(cola::CoLaParser::OpNeqContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitOpLt(cola::CoLaParser::OpLtContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitOpLte(cola::CoLaParser::OpLteContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitOpGt(cola::CoLaParser::OpGtContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitOpGte(cola::CoLaParser::OpGteContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitOpAnd(cola::CoLaParser::OpAndContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitOpOr(cola::CoLaParser::OpOrContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitValueNull(cola::CoLaParser::ValueNullContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitValueTrue(cola::CoLaParser::ValueTrueContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitValueFalse(cola::CoLaParser::ValueFalseContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitValueNDet(cola::CoLaParser::ValueNDetContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitValueEmpty(cola::CoLaParser::ValueEmptyContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitValueMin(cola::CoLaParser::ValueMinContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitValueMax(cola::CoLaParser::ValueMaxContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitExprValue(cola::CoLaParser::ExprValueContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitExprBinary(cola::CoLaParser::ExprBinaryContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitExprIdentifier(cola::CoLaParser::ExprIdentifierContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitExprParens(cola::CoLaParser::ExprParensContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitExprDeref(cola::CoLaParser::ExprDerefContext* /*context*/) override { throw std::logic_error("not implemented"); }
	    	antlrcpp::Any visitExprCas(cola::CoLaParser::ExprCasContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitExprNegation(cola::CoLaParser::ExprNegationContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitInvExpr(cola::CoLaParser::InvExprContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitInvActive(cola::CoLaParser::InvActiveContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitAngelChoose(cola::CoLaParser::AngelChooseContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitAngelActive(cola::CoLaParser::AngelActiveContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitAngelChooseActive(cola::CoLaParser::AngelChooseActiveContext* /*context*/) override { throw std::logic_error("not implemented"); }
			antlrcpp::Any visitAngelContains(cola::CoLaParser::AngelContainsContext* /*context*/) override { throw std::logic_error("not implemented"); }

			antlrcpp::Any visitOption(cola::CoLaParser::OptionContext* /*context*/) override { throw std::logic_error("not implemented"); }
	};

} // namespace cola
