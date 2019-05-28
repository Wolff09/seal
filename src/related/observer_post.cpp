#include <set>

#include "cola/ast.hpp"
#include "cola/observer.hpp"

#include "types/factory.hpp"


//#
//# TYPE DEFS
//#

using value_t = std::size_t;
using Memory = std::map<const cola::VariableDeclaration*, value_t>;
using Valuation = std::map<const cola::ObserverVariable*, value_t>;


//#
//# HELPERS
//#

struct IdentEvaluator final : public cola::ObserverVisitor {
	void visit(const cola::TrueGuard& /*obj*/) override { throw std::logic_error("Unexpected call to IdentEvaluator::visit(const cola::TrueGuard&)."); }
	void visit(const cola::ConjunctionGuard& /*obj*/) override { throw std::logic_error("Unexpected call to IdentEvaluator::visit(const cola::ConjunctionGuard&)."); }
	void visit(const cola::EqGuard& /*obj*/) override { throw std::logic_error("Unexpected call to IdentEvaluator::visit(const cola::EqGuard&)."); }
	void visit(const cola::NeqGuard& /*obj*/) override { throw std::logic_error("Unexpected call to IdentEvaluator::visit(const cola::NeqGuard&)."); }
	void visit(const cola::State& /*obj*/) override { throw std::logic_error("Unexpected call to IdentEvaluator::visit(const cola::State&)."); }
	void visit(const cola::Transition& /*obj*/) override { throw std::logic_error("Unexpected call to IdentEvaluator::visit(const cola::Transition&)."); }
	void visit(const cola::Observer& /*obj*/) override { throw std::logic_error("Unexpected call to IdentEvaluator::visit(const cola::Observer&)."); }

	const Valuation& obs2val;
	const Memory& var2val;
	const value_t executing_thread;
	value_t result;

	IdentEvaluator(const Valuation& obs2val, const Memory& var2val, value_t executing_thread) : obs2val(obs2val), var2val(var2val), executing_thread(executing_thread) {}

	void visit(const cola::ThreadObserverVariable& var) override { assert(this->obs2val.count(&var) != 0); this->result = this->obs2val.at(&var); }
	void visit(const cola::ProgramObserverVariable& var) override { assert(this->obs2val.count(&var) != 0); this->result = this->obs2val.at(&var); }
	void visit(const cola::SelfGuardVariable& /*var*/) override { this->result = this->executing_thread; }
	void visit(const cola::ArgumentGuardVariable& var) override { assert(this->var2val.count(&var.decl) != 0); this->result = this->var2val.at(&var.decl); }

	template<class V>
	value_t eval(const V& var) {
		var.accept(*this);
		return this->result;
	}
};

struct GuardEvaluator final : public cola::ObserverVisitor {
	void visit(const cola::ThreadObserverVariable& /*obj*/) override { throw std::logic_error("Unexpected call to GuardEvaluator::visit(const cola::ThreadObserverVariable&)."); }
	void visit(const cola::ProgramObserverVariable& /*obj*/) override { throw std::logic_error("Unexpected call to GuardEvaluator::visit(const cola::ProgramObserverVariable&)."); }
	void visit(const cola::SelfGuardVariable& /*obj*/) override { throw std::logic_error("Unexpected call to GuardEvaluator::visit(const cola::SelfGuardVariable&)."); }
	void visit(const cola::ArgumentGuardVariable& /*obj*/) override { throw std::logic_error("Unexpected call to GuardEvaluator::visit(const cola::ArgumentGuardVariable&)."); }
	void visit(const cola::State& /*obj*/) override { throw std::logic_error("Unexpected call to GuardEvaluator::visit(const cola::State&)."); }
	void visit(const cola::Transition& /*obj*/) override { throw std::logic_error("Unexpected call to GuardEvaluator::visit(const cola::Transition&)."); }
	void visit(const cola::Observer& /*obj*/) override { throw std::logic_error("Unexpected call to GuardEvaluator::visit(const cola::Observer&)."); }

	IdentEvaluator evaluator;
	bool result = true;

	GuardEvaluator(const Valuation& obs2val, const Memory& var2val, value_t executing_thread) : evaluator(obs2val, var2val, executing_thread) {}

	void visit(const cola::TrueGuard& /*guard*/) override {
		this->result = true;
	}
	void visit(const cola::EqGuard& guard) override {
		assert(guard.rhs);
		this->result = this->evaluator.eval(guard.lhs) == this->evaluator.eval(*guard.rhs);
	}
	void visit(const cola::NeqGuard& guard) override {
		assert(guard.rhs);
		this->result = this->evaluator.eval(guard.lhs) != this->evaluator.eval(*guard.rhs);
	}
	void visit(const cola::ConjunctionGuard& guard) override {
		this->result = true;
		for (const auto& clause : guard.conjuncts) {
			assert(clause);
			clause->accept(*this);
			if (this->result == false) {
				break;
			}
		}
	}

	bool is_enabled(const cola::Guard& guard) {
		guard.accept(*this);
		return this->result;
	}
};

std::set<const cola::State*> post(const cola::Observer& observer, const cola::State& current_state, const cola::Function& function, cola::Transition::Kind kind, value_t executing_thread, const Memory& var2adr, const Valuation& obs2val) {
	GuardEvaluator evaluator(obs2val, var2adr, executing_thread);
	std::set<const cola::State*> result;
	for (const auto& transition : observer.transitions) {
		assert(transition->guard);
		if (&transition->src == &current_state && &transition->label == &function && transition->kind == kind && evaluator.is_enabled(*transition->guard)) {
			result.insert(&transition->dst);
		}
	}
	return result;
}

Memory mk_param_map(const cola::Function& function, const std::vector<value_t>& params) {
	assert(params.size() == function.args.size());
	Memory result;
	for (std::size_t index = 0; index < params.size(); ++index) {
		result[function.args.at(index).get()] = params.at(index);
	}
	return result;
}


//#
//# PUBLIC INTERFACE
//#

std::set<const cola::State*> observer_post_enter(const cola::Observer& observer, const cola::State& current_state, const cola::Enter& command, value_t executing_thread, const std::vector<value_t>& params, const Valuation& obs2val) {
	auto var2adr = mk_param_map(command.decl, params);
	return post(observer, current_state, command.decl, cola::Transition::INVOCATION, executing_thread, var2adr, obs2val);
}

std::set<const cola::State*> observer_post_exit(const cola::Observer& observer, const cola::State& current_state, const cola::Exit& command, value_t executing_thread, const Valuation& obs2val) {
	return post(observer, current_state, command.decl, cola::Transition::RESPONSE, executing_thread, {}, obs2val);
}

std::set<const cola::State*> observer_post_free(const cola::Observer& observer, const cola::State& current_state, const value_t adr_to_free, const Valuation& obs2val) {
	static value_t executing_thread = 0; // some dummy value -> we assume that the observer transitions for 'free' do not need it
	const cola::Function& free_function = observer.free_function();
	assert(free_function.args.size() == 1);
	Memory var2adr;
	var2adr[free_function.args.at(0).get()] = adr_to_free;
	return post(observer, current_state, free_function, cola::Transition::INVOCATION, executing_thread, var2adr, obs2val);
}


//#
//# TESTS
//#

int main(int /*argc*/, char** /*argv*/) {
	// auto hpobs = make_hp_no_transfer_observer(retire_func, protect_func, "test");
	return -1;
}

