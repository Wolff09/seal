#include "types/guarantees.hpp"
#include "types/factory.hpp"
#include "types/inference.hpp"
#include <deque>

using namespace cola;
using namespace prtypes;


std::unique_ptr<Guarantee> make_guarantee(const SmrObserverStore& store, std::string name, bool transient, bool valid) {
	auto result = std::make_unique<Guarantee>(name, prtypes::make_active_local_guarantee_observer(store.retire_function, name));
	result->is_transient = transient;
	result->entails_validity = valid;
	return result;
}

std::unique_ptr<Guarantee> make_active_guarantee(const SmrObserverStore& store) {
	return make_guarantee(store, "Active", true, true);
}

std::unique_ptr<Guarantee> make_local_guarantee(const SmrObserverStore& store) {
	return make_guarantee(store, "Local", false, true);
}

GuaranteeTable::GuaranteeTable(const SmrObserverStore& store) : observer_store(store) {
	// TODO: perform checks on active/local guarantees
	all_guarantees.push_back(make_active_guarantee(store));
	all_guarantees.push_back(make_local_guarantee(store));
}

const Guarantee& GuaranteeTable::active_guarantee() const {
	assert(all_guarantees.size() > 0);
	return *all_guarantees.at(0).get();
}

const Guarantee& GuaranteeTable::local_guarantee() const {
	assert(all_guarantees.size() > 1);
	return *all_guarantees.at(1).get();
}


struct InferferenceVisitor final : public ObserverVisitor {
	bool no_interference = false;
	bool is_self = false;
	void visit(const SelfGuardVariable& /*obj*/) { is_self = true; }
	void visit(const ConjunctionGuard& guard) {
		for (const auto& subguard : guard.conjuncts) {
			subguard->accept(*this);
		}
	}
	void visit(const EqGuard& guard) {
		is_self = false;
		guard.rhs->accept(*this);
		if (is_self) {
			no_interference = true;
		}
	}
	void visit(const ArgumentGuardVariable& /*obj*/) { /* do nothing */ }
	void visit(const TrueGuard& /*obj*/) { /* do nothing */ }
	void visit(const NeqGuard& /*obj*/) { /* do nothing */ }
	void visit(const ThreadObserverVariable& /*obj*/) { throw std::logic_error("Unexpected invocation: InferferenceVisitor::visit(const ThreadObserverVariable&)"); }
	void visit(const ProgramObserverVariable& /*obj*/) { throw std::logic_error("Unexpected invocation: InferferenceVisitor::visit(const ProgramObserverVariable&)"); }
	void visit(const State& /*obj*/) { throw std::logic_error("Unexpected invocation: InferferenceVisitor::visit(const State&)"); }
	void visit(const Transition& /*obj*/) { throw std::logic_error("Unexpected invocation: InferferenceVisitor::visit(const Transition&)"); }
	void visit(const Observer& /*obj*/) { throw std::logic_error("Unexpected invocation: InferferenceVisitor::visit(const Observer&)"); }
};

bool is_guarantee_transient(const Observer& observer) {
	auto is_state_final = [&](const State& state) -> bool {
		return state.final == !observer.negative_specification;
	};
	auto may_interfere = [](const Guard& guard) -> bool {
		InferferenceVisitor visitor;
		guard.accept(visitor);
		return !visitor.no_interference;
	};

	for (const auto& transition : observer.transitions) {
		if (is_state_final(transition->src) && may_interfere(*transition->guard) && !is_state_final(transition->dst)) {
			return true;
		}
	}
	return false;

	// std::set<const State*> reachable;
	// std::deque<const State*> worklist;

	// // add final states
	// for (const auto& state : observer.states) {
	// 	if (is_state_final(*state)) {
	// 		reachable.insert(state.get());
	// 		worklist.push_back(state.get());
	// 	}
	// }

	// // explore reachable via interference events
	// while (!worklist.empty()) {
	// 	const State& current = *worklist.front();
	// 	worklist.pop_front();

	// 	for (const auto& transition : observer.transitions) {
	// 		if (&transition->src == &current && may_interfere(*transition->guard)) {
	// 			auto insertion = reachable.insert(&transition->dst);
	// 			if (insertion.second) {
	// 				worklist.push_back(&transition->dst);
	// 			}
	// 		}
	// 	}
	// }

	// // check reachable
	// for (const auto& state : reachable) {
	// 	if (!is_state_final(*state)) {
	// 		return true;
	// 	}
	// }
	// return false;
}

bool entails_guarantee_validity(const Guarantee& guarantee) {
	if (guarantee.is_transient) {
		// our check relies on the guarantee being non-transient (we explore just one free step)
		return false;
	}

	throw std::logic_error("not yet implemented: entails_guarantee_validity");

	// where to get the program from ?
	// how to create a dummy Guarantee Table?
	// Translator translator(program, table);
			// 	const VataNfa& nfa_for(const Guarantee& guarantee) const { return guarantee2nfa.at(&guarantee); }
			
			// VataNfa to_nfa(const cola::Observer& observer);


	// auto is_state_final = [&](const State& state) -> bool {
	// 	return state.final == !guarantee.observer->negative_specification;
	// };

	// // check if a final state can reach a final state using free, if so return false
	// for (const auto& transition : guarantee.observer->transitions) {
	// 	if (is_state_final(transition->src) && &transition->label == &Observer::free_function() && is_state_final(transition->dst)) {
	// 		return false;
	// 	}
	// }
	// return true;
}

const Guarantee& GuaranteeTable::add_guarantee(std::unique_ptr<Observer> observer, std::string name) {
	// create guarantee
	raise_if_assumption_unsatisfied(*observer);
	auto guarantee = std::make_unique<Guarantee>(name, std::move(observer));
	this->all_guarantees.push_back(std::move(guarantee));
	Guarantee& result = *this->all_guarantees.back().get();
	// after adding, check its properties
	result.is_transient = is_guarantee_transient(*result.observer);
	result.entails_validity = entails_guarantee_validity(result);
	return result;
}
