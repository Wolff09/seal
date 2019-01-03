#include "util.hpp"

using namespace cola;


struct ObserverCheckVisitor final : public ObserverVisitor {
	std::size_t cVoid = 0, cBool = 0, cData = 0, cPtr = 0, cThread = 0;

	void visit(const Observer& observer) override {
		for (const auto& ovar : observer.variables) {
			ovar->accept(*this);
		}
		auto mkError = [](std::string sort, std::string max) {
			return std::logic_error("Unsupported observer: exceeds allowed number of observed enteties of " + sort + " sort; allowed maximum is '" + max + "'.");
		};
		if (cVoid != 0) {
			throw mkError("'void'", "0");
		}
		if (cBool != 0) {
			throw mkError("'bool'", "0");
		}
		if (cData != 0) {
			throw mkError("data", "0");
		}
		if (cPtr > 1) {
			throw mkError("pointer", "1");
		}
		if (cThread > 1) {
			throw mkError("thread", "1");
		}
	}

	void visit(const ThreadObserverVariable& /*variable*/) override {
		cThread++;
	}

	void visit(const ProgramObserverVariable& variable) override {
		switch(variable.decl->type.sort) {
			case Sort::VOID: cVoid++; break;
			case Sort::BOOL: cBool++; break;
			case Sort::DATA: cData++; break;
			case Sort::PTR: cPtr++; break;
		}
	}

	void visit(const SelfGuardVariable& /*obj*/) override { throw std::logic_error("Unexpected invocation (ObserverCheckVisitor::visit(const SelfGuardVariable&))"); }
	void visit(const ArgumentGuardVariable& /*obj*/) override { throw std::logic_error("Unexpected invocation (ObserverCheckVisitor::visit(const ArgumentGuardVariable&))"); }
	void visit(const BoolGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation (ObserverCheckVisitor::visit(const BoolGuard&))"); }
	void visit(const ConjunctionGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation (ObserverCheckVisitor::visit(const ConjunctionGuard&))"); }
	void visit(const EqGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation (ObserverCheckVisitor::visit(const EqGuard&))"); }
	void visit(const NeqGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation (ObserverCheckVisitor::visit(const NeqGuard&))"); }
	void visit(const State& /*obj*/) override { throw std::logic_error("Unexpected invocation (ObserverCheckVisitor::visit(const State&))"); }
	void visit(const Transition& /*obj*/) override { throw std::logic_error("Unexpected invocation (ObserverCheckVisitor::visit(const Transition&))"); }
};


void prtypes::observer_to_symbolic_nfa(const cola::Observer& observer) {
	// ensure that observer observes at most one thread and one pointer (required for correctness of transformation)
	ObserverCheckVisitor().visit(observer);

	

	throw std::logic_error("not yet implemented (observer_to_symbolic_nfa)");
}