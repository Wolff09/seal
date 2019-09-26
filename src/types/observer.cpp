#include "types/observer.hpp"
#include "types/assumption.hpp"
#include <iostream>

using namespace cola;
using namespace prtypes;


//
// SMR store
//
SmrObserverStore::SmrObserverStore(const cola::Program& program, const cola::Function& retire_function) : program(program), retire_function(retire_function), base_observer(prtypes::make_base_smr_observer(retire_function)) {
	this->simulation.compute_simulation(*this->base_observer);
	prtypes::raise_if_assumption_unsatisfied(*this->base_observer);
	// conditionally_raise_error<UnsupportedObserverError>(!supports_elison(*this->base_observer), "does not support elision");
	// NOTE: shown manually that base_observer supports elision?
}

void SmrObserverStore::add_impl_observer(std::unique_ptr<cola::Observer> observer) {
	prtypes::raise_if_assumption_unsatisfied(*observer);
	this->impl_observer.push_back(std::move(observer));
	this->simulation.compute_simulation(*this->impl_observer.back());
	conditionally_raise_error<UnsupportedObserverError>(!supports_elison(*this->impl_observer.back()), "does not support elision");
}

bool SmrObserverStore::supports_elison(const Observer& observer) const {
	// requires simulation to be computed

	prtypes::raise_if_assumption_unsatisfied(observer);
	// assumption ==> free(<unobserved>) has not effect
	// assumption ==> swapping <unobserved> addresses has no effect
	// remains to check: <any state> in simulation relation with <initial state>

	for (const auto& state : observer.states) {
		if (state->initial) {
			for (const auto& other : observer.states) {
				if (!simulation.is_in_simulation_relation(*other, *state)) {
					return false;
				}
			}	
		}
	}
	return true;
}


//
// combining states
//
// template<typename T> // T = container<const SymbolicState*>
// inline SymbolicStateSet combine_states(std::vector<T> list) {
// 	std::vector<typename T::const_iterator> begin, cur, end;
// 	bool available = true;
// 	for (std::size_t index = 0; index < list.size(); ++index) {
// 		const auto& elem = list.at(index);
// 		begin.push_back(elem.begin());
// 		cur.push_back(elem.begin());
// 		end.push_back(elem.end());
// 		available &= elem.begin() != elem.end();
// 	}

// 	SymbolicStateSet result;
// 	while (available) {
// 		// get current vector
// 		StateVec element;
// 		for (const auto& it : cur) {
// 			element.push_back(*it);
// 		}
// 		result.insert(element);

// 		// progress cur
// 		available = false;
// 		for (std::size_t index = 0; index < cur.size(); ++index) {
// 			cur.at(index)++;

// 			if (cur.at(index) == end.at(index)) {
// 				cur.at(index) = begin.at(index);
// 			} else {
// 				available = true;
// 				break;
// 			}
// 		}
// 	}

// 	return result;
// }
template<typename T> // T = container<const SymbolicState*>
struct CombinationMaker {
	CombinationMaker(const std::vector<std::set<T>> vector);
	bool available() const;
	std::vector<T> get_next();
};


//
// common helpers
//
inline bool could_be_sat(z3::result result) {
	switch (result) {
		case z3::unsat: return false;
		case z3::unknown: return true;
		case z3::sat: return true;
	}
	assert(false);
}

template<typename Functor>
void apply_to_observers(const SmrObserverStore& store, Functor& func) {
	func(*store.base_observer);
	for (const auto& observer : store.impl_observer) {
		func(*observer);
	}
}

template<typename Functor>
void apply_to_states(const SmrObserverStore& store, Functor& func) {
	apply_to_observers(store, [&func](const Observer& observer) {
		for (const auto& state : observer.states) {
			func(*transition);
		}
	});
}

template<typename Functor>
void apply_to_transitions(const SmrObserverStore& store, Functor& func) {
	apply_to_states(store, [&func](const State& state) {
		for (const auto& transition : state.transitions) {
			func(*transition);
		}
	});
}

inline bool has_nonempty_intersection(const std::vector<const State*>& states, const std::set<const State*>& other) {
	for (const State* state : states) {
		if (other.count(state) > 0) {
			return true;
		}
	}
	return false;
}


//
// SymbolicObserver construction
//
inline bool needs_closure(const SymbolicObserver& observer, const SymbolicTransition& transition) {
	observer.solver.push();
	observer.solver.add(transition.guard);
	observer.solver.add(observer.selfparam != observer.threadvar);
	auto check_result = translation.solver.check();
	observer.solver.pop();
	return could_be_sat(check_result);
}

inline SymbolicStateSet compute_closure(const SymbolicObserver& observer, const SymbolicStateSet& set) {
	SymbolicStateSet result(set);
	std::deque<const SymbolicState*> worklist(set);
	while (!worklist.empty()) {
		for (const SymbolicTransition& transition : worklist.front().transitions) {
			if (needs_closure(observer, transition)) {
				auto insertion = result.insert(&transition.dst);
				if (insertion.second) {
					worklist.push_back(&transition.dst);
				}
			}
		}
		worklist.pop_front();
	}
	return result;
}

struct GuardTranslator final : public ObserverVisitor {
	const SymbolicObserver& observer
	z3::expr result;
	std::map<const VariableDeclaration*, std::size_t> param2index;
	
	GuardTranslator(const SymbolicObserver& observer, const Function& function) : observer(observer), result(context.bool_val(true)) {
		for (std::size_t index = 0; index < function.args.size(); ++index) {
			param2index[function.args.at(index).get()] = index;
		}
	}

	void visit(const State& /*obj*/) override { throw std::logic_error("Unexpected invocation (GuardTranslator::void visit(const State&))"); }
	void visit(const Transition& /*obj*/) override { throw std::logic_error("Unexpected invocation (GuardTranslator::void visit(const Transition&))"); }
	void visit(const Observer& /*obj*/) override { throw std::logic_error("Unexpected invocation (GuardTranslator::void visit(const Observer&))"); }

	void visit(const ThreadObserverVariable& /*var*/) override { result = observer.threadvar; }
	void visit(const ProgramObserverVariable& /*var*/) override { result = observer.adrvar; }
	void visit(const SelfGuardVariable& /*var*/) override { result = observer.selfparam; }
	void visit(const ArgumentGuardVariable& var) override { result = observer.paramvars.at(param2index.at(&var.decl)); }
	void visit(const TrueGuard& /*obj*/) override { result = context.bool_val(true); }
	void mk_equality(const ComparisonGuard& guard) {
		guard.lhs.accept(*this);
		z3::expr lhs = result;
		guard.rhs->accept(*this);
		z3::expr rhs = result;
		result = (lhs == rhs);
	}
	void visit(const EqGuard& guard) override {
		mk_equality(guard);
	}
	void visit(const NeqGuard& guard) override {
		mk_equality(guard);
		result = !result;
	}
	void visit(const ConjunctionGuard& guard) override {
		z3::expr_vector conjuncts(context);
		for (const auto& conj : guard.conjuncts) {
			conj->accept(*this);
			conjuncts.push_back(result);
		}
		result = z3::mk_and(conjuncts);
	}
};


inline z3::expr translate_guard(const SymbolicObserver& observer, const Transition& transition) {
	GuardTranslator translator(observer, transition.label);
	transition.guard.accept(translator);
	return translator.result);
}

SymbolicTransition::SymbolicTransition(const SymbolicState& dst, const cola::Transition& transition) : dst(dst), label(transition.label), kind(transition.kind), guard(translate_guard(dst.observer, transition)) {
}

SymbolicState::SymbolicState(const SymbolicObserver& observer, bool is_final, bool is_active) : observer(observer), is_final(is_final), is_active(is_active) {
}

inline std::size_t find_max_params(const SmrObserverStore& store) {
	std::size_t max = 0;
	apply_to_transitions(store, [&max](const Transition& transition) {
		max = std:max(max, transition.label.args.size());
	});
	return max;
}

inline std::set<std::pair<const cola::Function*, Transition::Kind>> collect_transition_infos(const SmrObserverStore& store) {
	std::set<std::pair<const cola::Function*, Transition::Kind>> result;
	apply_to_transitions(store, [&max](const Transition& transition) {
		result.insert({ &transition.label, transition.kind });
	});
	return result;
}

inline std::size_t guess_final_size(const SmrObserverStore& store) {
	std::size_t result = 1;
	apply_to_transitions(store, [&max](const Transition& transition) {
		result *= observer.states.size();
	});
	return result;
}

struct HalfWaySymbolicTransition {
	const State& dst;
	const cola::Function& label;
	cola::Transition::Kind kind;
	z3::expr guard;
	
	HalfWaySymbolicTransition(const SymbolicObserver& observer, const Transition& transition) : dst(transition.dst), label(transition.label), kind(transition.kind), guard(translate_guard(observer, transition)) {
	}
	HalfWaySymbolicTransition(const State& dst, const Function& label, Transition::Kind kind, z3::expr guard) : dst(dst), label(label), kind(kind), guard(guard) {
	}
};

inline std::map<const State*, std::list<HalfWaySymbolicTransition>> make_complete_transition_map(const SymbolicObserver& observer, const SmrObserverStore& store, const std::set<std::pair<const Function*, Transition::Kind>>& all_symbols) {
	std::map<const State*, std::list<HalfWaySymbolicTransition>> result;
	apply_to_transitions(store, [&max](const Transition& transition) {
		result[&transition.src].emplace_back(observer, transition);
	});

	for (auto& [state, transitions] : result) {
		for (const auto& [label, kind] : all_symbols) {
			z3::expr_vector remaining(observer.context);

			// collect negated guards from all matching transistions
			for (const auto& transition : transitions) {
				if (&transition.label == label && transition.kind == kind) {
					remaining.push_back(!transition.guard);
				}
			}

			// add transition completion (if necessary)
			z3::expr remaining_guard = z3::mk_and(remaining)
			observer.solver.push();
			auto check_result = translation.solver.check();
			observer.solver.pop();
			if (could_be_sat(check_result)) {
				transitions.emplace_back(*state, label, kind, remaining_guard);
			}
		}
	}
}

struct CrossProductMaker {
	const SmrObserverStore& store;
	const SymbolicObserver& observer;
	const std::set<const State*>& active_states, final_states;

	std::vector<std::unique_ptr<SymbolicState>> states;
	std::deque<SymbolicState*> worklist;
	std::map<std::vector<const cola::State*>, SymbolicState*> state2symbolic;
	std::set<std::pair<const Function*, Transition::Kind>> all_symbols;
	std::map<const State*, std::list<HalfWaySymbolicTransition>> state2transition; // TODO: use list instead of set?


	CrossProductMaker(const SmrObserverStore& store, const SymbolicObserver& observer, const std::set<const State*>& active_states, const std::set<const State*>& final_states)
	 	: store(store), observer(observer), active_states(active_states), final_states(final_states), all_symbols(collect_transition_infos(store)), state2transition(make_complete_transition_map(observer, store, all_symbols))
	{
		states.reserve(guess_final_size(store));
	}

	SymbolicState* add_state(std::vector<const cola::State*> state) {
		states.push_back(std::make_unique<SymbolicState>(observer, has_nonempty_intersection(state, final_states), has_nonempty_intersection(state, active_states)));
		states.back()->origin = state;
		auto insertion = state2symbolic.insert({ std::move(state), states.back().get() });
		assert(insertion.second);
		return states.back().get();
	}

	SymbolicState* add_or_get_state(std::vector<const cola::State*> state) {
		auto find = state2symbolic.find(state);
		if (find == state2symbolic.end()) {
			return add_state(std::move(state));
		} else {
			return *find->second;
		}
	}

	void prepare_initial_states() {
		// find initial states per observer
		std::vector<std::set<const State*>> initial_states;
		apply_to_observers(store, [&initial_states](const cola::Observer& observer) {
			std::set<const State*> init;
			for (const auto& state : observer.states) {
				if (state->initial) {
					init.insert(state.get());
				}
			}
			initial_states.push_back(std::move(init));
		});

		// create all combination of per-observer initial states
		CombinationMaker maker(initial_states);
		while (maker.available()) {
			auto new_state = add_or_get_state(maker.get_next());
			worklist.push_back(new_state);
		}
	}

	void handle_symbolicstate(SymbolicState* state) {
		throw std::logic_error("not yet implemented"); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<---------------------------------------------------------------|||||||||||||||||||||
		// post image für alle symbole; transitionen vervollständigen
	}

	void compute_cross_product() {
		prepare_initial_states();

		while (!worklist.empty()) {
			SymbolicState* current = worklist.front();
			worklist.pop_front();

			handle_symbolicstate(current);
		}
	}
};

inline std::vector<SymbolicState> make_states(const SmrObserverStore& store, const SymbolicObserver& observer) {
	std::set<const State*> active_states;
	for (const auto& state : store.base_observer) {
		if (state->initial) {
			active_states.insert(state.get());
		}
	}

	CrossProductMaker maker(store, observer);
	maker.compute_cross_product();
	return std::move(maker.states);
}

SymbolicObserver::SymbolicObserver(const SmrObserverStore& store) : solver(context) {
	// add variables to context
	this->threadvar = context.int_const("__THREAD");
	this->adrvar = context.int_const("__ADR");
	this->selfparam = context.int_const("self");
	for (std::size_t index = 0; index < find_max_params(store); ++index) {
		this->selfparam = context.int_const("param_" + std::to_string(index));
	}

	// compute cross product
	this->states = make_states(store, *this);

	// compute closures
	for (SymbolicState& state : this->states) {
		state.closure = compute_closure(*this, { state });
	}
}


//
// SymbolicObserver operations
//
inline bool is_equal(const Expression& expression, const Expression& other) {
	const cola::VariableExpression* var_expression = dynamic_cast<const cola::VariableExpression*>(expression);
	const cola::VariableExpression* var_other = dynamic_cast<const cola::VariableExpression*>(other);
	if (var_expression && var_other) {
		return &var_expression.decl == &var_other.decl;
	}
	throw std::logic_error("Cannot compare expressions; must be 'VariableExpression'.");
}

inline std::tuple<const cola::Function*, cola::Transition::Kind, z3::expr> prepare(const SymbolicObserver& observer, const cola::Enter& command, const cola::VariableDeclaration& variable) {
	z3::expr_vector constraints(observer.context);

	// force post for the executing threads
	constraints.push_back(observer.selfparam == observer.threadvar);

	// map command argument equalities to command.decl.args equalities
	for (std::size_t index = 0; index < command.args.size(); ++index) {
		for (std::size_t other = index + 1; other < command.args.size(); ++other) {
			if (is_equal(*command.args.at(index), *command.args.at(other))) {
				constraints.push_back(observer.paramvars.at(index) == observer.paramvars.at(other));
			}
		}
	}

	// map command argument-variable equalities to command.decl.args-adrvar equalities
	auto dummy_var_expression = std::make_unique<VariableExpression>(variable);
	for (std::size_t index = 0; index < command.args.size(); ++index) {
		if (is_equal(*command.args.at(index), *dummy_var_expression)) {
			constraint.push_back(observer.paramvars.at(index) == observer.adrvar);
		}
	}

	// put result together
	return (command.decl, cola::Transition::INVOCATION, z3::mk_and(constraints););
}

inline std::tuple<const cola::Function*, cola::Transition::Kind, z3::expr> prepare(const SymbolicObserver& observer, const cola::Exit& command, const cola::VariableDeclaration& variable) {
	return (command.decl, cola::Transition::RESPONSE, make_selfparam_constraint());
}

inline std::tuple<const cola::Function*, cola::Transition::Kind, z3::expr> prepare(const SymbolicObserver& observer, const cola::Command& command, const cola::VariableDeclaration& variable) {
	const cola::Enter* enter = dynamic_cast<const cola::Enter*>(&command);
	if (enter) {
		return prepare(observer, *enter, variable);
	}
	const cola::Exit* exit = dynamic_cast<const cola::Exit*>(&command);
	if (exit) {
		return prepare(observer, *exit, variable);
	}
	throw std::logic_error("Cannot compute post image for command; must be 'Enter' or 'Exit'.");
}

inline bool matches(const SymbolicTransition& transition, const cola::Function* label, cola::Transition::Kind kind) {
	return &transition.label == &label && transition.kind == kind;
}


SymbolicStateSet prtypes::symbolic_post(const SymbolicState& state, const cola::Command& command, const cola::VariableDeclaration& variable) {
	const auto& [label, kind, constraint] = prepare(observer, command, variable);
	observer.solver.push();
	observer.solver.add(constraint);

	SymbolicStateSet result;
	for (const SymbolicTransition& transition : state.transitions) {
		if (matches(transition, label, kind) && result.count(&transition.dst) == 0) {
			observer.solver.push();
			observer.solver.add(transition.guard);
			auto check_result = translation.solver.check();
			observer.sovler.pop();
			if (could_be_sat(check_result)) {
				result.insert(&transition.dst);
			}
		}
	}

	observer.solver.pop();
	return result;
}

SymbolicStateSet prtypes::symbolic_post(const SymbolicStateSet& set, const cola::Command& command, const cola::VariableDeclaration& variable) {
	SymbolicStateSet result;
	for (const SymbolicState* state : set) {
		auto post = prtypes::symbolic_post(*state, command, variable);
		result.insert(post.begin(), post.end());
	}
	return result;
}

SymbolicStateSet prtypes::symbolic_closure(const SymbolicStateSet& set) {
	SymbolicStateSet result;
	for (const SymbolicState& state : set) {
		result.insert(state.closure.begin(), state.closure.end());
	}
	return result;
}

SymbolicStateSet prtypes::symbolic_closure(const SymbolicState& state) {
	return state.closure;
}
