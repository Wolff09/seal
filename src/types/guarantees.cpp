#include "types/guarantees.hpp"
#include "types/factory.hpp"
#include "types/inference.hpp"
#include "cola/util.hpp"
#include "vata2/nfa.hh"
#include <deque>
#include <array>

using namespace cola;
using namespace prtypes;


std::unique_ptr<Guarantee> copy_guarantee(const Guarantee& guarantee) {
	auto observer = cola::copy(*guarantee.observer);
	auto result = std::make_unique<Guarantee>(guarantee.name, std::move(observer));
	result->is_transient = guarantee.is_transient;
	result->entails_validity = guarantee.entails_validity;
	return result;
}

std::unique_ptr<Guarantee> make_guarantee(const SmrObserverStore& store, std::string name, bool transient, bool valid) {
	auto observer = prtypes::make_active_local_guarantee_observer(store.retire_function, name);
	auto result = std::make_unique<Guarantee>(name, std::move(observer));
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

bool is_observer_transient(const Observer& observer) {
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
}

bool is_guarantee_transient(const Guarantee& guarantee) {
	assert(!guarantee.observer->negative_specification); // TODO: raise exception if not satisfied
	if (!is_observer_transient(*guarantee.observer)) {
		return false;
	}
	return true;
}

struct FindObservedAddressVisitor final : public ObserverVisitor {
	const ProgramObserverVariable* result = nullptr;
	void visit(const SelfGuardVariable& /*obj*/) { throw std::logic_error("Unexpected invocation: FindObservedAddressVisitor::visit(const SelfGuardVariable&)"); }
	void visit(const ArgumentGuardVariable& /*obj*/) { throw std::logic_error("Unexpected invocation: FindObservedAddressVisitor::visit(const ArgumentGuardVariable&)"); }
	void visit(const TrueGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: FindObservedAddressVisitor::visit(const TrueGuard&)"); }
	void visit(const ConjunctionGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: FindObservedAddressVisitor::visit(const ConjunctionGuard&)"); }
	void visit(const EqGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: FindObservedAddressVisitor::visit(const EqGuard&)"); }
	void visit(const NeqGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: FindObservedAddressVisitor::visit(const NeqGuard&)"); }
	void visit(const State& /*obj*/) { throw std::logic_error("Unexpected invocation: FindObservedAddressVisitor::visit(const State&)"); }
	void visit(const Transition& /*obj*/) { throw std::logic_error("Unexpected invocation: FindObservedAddressVisitor::visit(const Transition&)"); }
	void visit(const ThreadObserverVariable& /*obj*/) { /* do nothing*/ }
	void visit(const ProgramObserverVariable& var) {
		result = &var;
	}
	void visit(const Observer& observer) {
		for (const auto& var : observer.variables) {
			var->accept(*this);
		}
	}
};

Transition make_dummy_free_transition(const Observer& observer) {
	assert(observer.states.size() > 0);
	FindObservedAddressVisitor visitor;
	observer.accept(visitor);
	assert(visitor.result);
	auto& free_function = Observer::free_function();
	assert(free_function.args.size() == 1);
	auto guard = std::make_unique<EqGuard>(*visitor.result, std::make_unique<ArgumentGuardVariable>(*free_function.args.at(0)));
	auto& dummy_state = *observer.states.at(0);
	return Transition(dummy_state, dummy_state, free_function, Transition::INVOCATION, std::move(guard));
}

bool entails_guarantee_validity(const SmrObserverStore& store, const Guarantee& guarantee) {
	if (guarantee.is_transient) {
		// our check relies on the guarantee being non-transient (we explore just one free step)
		return false;
	}

	// dummy translator
	std::array<std::reference_wrapper<const Guarantee>, 1> dummy_container = { guarantee };
	Translator translator(store.program, dummy_container);
	
	// get guarantee observer language, concate with free(*)
	auto intersection = translator.nfa_for(guarantee);
	intersection = translator.concatenate_step(intersection, make_dummy_free_transition(*guarantee.observer));

	// intersection with observer languages
	auto intersect = [&](const Observer& observer) {
		auto automaton = translator.to_nfa(observer);
		intersection = Vata2::Nfa::intersection(intersection, automaton);
	};

	// intersect with smr observer languages
	intersect(*store.base_observer);
	for (const auto& observer : store.impl_observer) {
		intersect(*observer);
	}

	// ensure only valid histories are considered
	intersection = Vata2::Nfa::intersection(intersection, translator.get_wellformedness_nfa());

	// return Vata2::Nfa::is_lang_empty(intersection);
	Vata2::Nfa::Word cex;
	bool result = Vata2::Nfa::is_lang_empty_cex(intersection, &cex);
	// if (!result) {
	// 	std::cout << "    ==> Does not entail validity, counterexample: ";
	// 	for (const auto& sym : cex) {
	// 		std::cout << " " << sym;
	// 	}
	// 	std::cout << std::endl;
	// }
	return result;
}

std::vector<std::reference_wrapper<const Guarantee>> GuaranteeTable::add_guarantee(std::unique_ptr<Observer> observer, std::string name) {
	raise_if_assumption_unsatisfied(*observer);
	std::vector<std::reference_wrapper<const Guarantee>> result;

	// create new guarantee
	auto guarantee = std::make_unique<Guarantee>(name, std::move(observer));
	guarantee->is_transient = is_guarantee_transient(*guarantee);
	guarantee->entails_validity = false;
	result.push_back(*guarantee.get());

	// add a copy of guarantee if it entails validity
	if (entails_guarantee_validity(observer_store, *guarantee)) {
		auto safe_guarantee = copy_guarantee(*guarantee);
		safe_guarantee->name += "-SAFE";
		safe_guarantee->entails_validity = true;
		result.push_back(*safe_guarantee.get());
		this->all_guarantees.push_back(std::move(safe_guarantee));
	}

	// add guarantee
	this->all_guarantees.push_back(std::move(guarantee));
	return result;
}
