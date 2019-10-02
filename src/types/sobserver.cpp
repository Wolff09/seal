#include "types/sobserver.hpp"

#include <iostream>
#include <deque>
#include <list>
#include "types/error.hpp"
#include "types/assumption.hpp"

using namespace cola;
using namespace prtypes;


//
// combining states
//

template<typename T> // T must be of pointer-type
class CombinationMaker {
	private:
		const std::vector<std::set<T>>& vector;
		std::vector<typename std::set<T>::const_iterator> begin, cur, end;
		bool next_available;

	public:
		CombinationMaker(const std::vector<std::set<T>>& vector) : vector(vector) {
			next_available = true;
			for (const auto& elem : vector) {
				begin.push_back(elem.begin());
				cur.push_back(elem.begin());
				end.push_back(elem.end());
				next_available &= elem.begin() != elem.end();
			}
		}

		bool available() const {
			return next_available;
		}

		std::vector<T> get_next() {
			// get current vector
			std::vector<T> result;
			for (const auto& it : cur) {
				result.push_back(*it);
			}

			// progress cur
			next_available = false;
			for (std::size_t index = 0; index < cur.size(); ++index) {
				cur.at(index)++;

				if (cur.at(index) == end.at(index)) {
					cur.at(index) = begin.at(index);
				} else {
					next_available = true;
					break;
				}
			}

			return result;
		}
};


//
// common helpers
//
struct Context {
	const SymbolicObserver& observer;
	z3::context& context;
	z3::solver& solver;

	Context(SymbolicObserver& observer, z3::context& context, z3::solver& solver) : observer(observer), context(context), solver(solver) {}
};

inline bool could_be_sat(z3::check_result result) {
	switch (result) {
		case z3::unsat: return false;
		case z3::unknown: return true;
		case z3::sat: return true;
	}
	assert(false);
}

template<typename Functor>
void apply_to_observers(const SmrObserverStore& store, Functor func) {
	func(*store.base_observer);
	for (const auto& observer : store.impl_observer) {
		func(*observer);
	}
}

template<typename Functor>
void apply_to_states(const SmrObserverStore& store, Functor func) {
	apply_to_observers(store, [&func](const Observer& observer) {
		for (const auto& state : observer.states) {
			func(*state);
		}
	});
}

template<typename Functor>
void apply_to_transitions(const SmrObserverStore& store, Functor func) {
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
inline bool needs_closure(Context context, const SymbolicTransition& transition) {
	context.solver.push();
	context.solver.add(transition.guard);
	context.solver.add(context.observer.selfparam != context.observer.threadvar);
	auto check_result = context.solver.check();
	context.solver.pop();
	return could_be_sat(check_result);
}

inline SymbolicStateSet compute_closure(Context context, const SymbolicStateSet& set) {
	SymbolicStateSet result(set);
	std::deque<const SymbolicState*> worklist(set.begin(), set.end());
	while (!worklist.empty()) {
		for (const auto& transition : worklist.front()->transitions) {
			if (needs_closure(context, *transition)) {
				auto insertion = result.insert(&transition->dst);
				if (insertion.second) {
					worklist.push_back(&transition->dst);
				}
			}
		}
		worklist.pop_front();
	}
	return result;
}

struct GuardTranslator final : public ObserverVisitor {
	Context context;
	z3::expr result;
	std::map<const VariableDeclaration*, std::size_t> param2index;
	
	GuardTranslator(Context context, const Function& function) : context(context), result(context.context.bool_val(true)) {
		for (std::size_t index = 0; index < function.args.size(); ++index) {
			param2index[function.args.at(index).get()] = index;
		}
	}

	void visit(const State& /*obj*/) override { throw std::logic_error("Unexpected invocation (GuardTranslator::void visit(const State&))"); }
	void visit(const Transition& /*obj*/) override { throw std::logic_error("Unexpected invocation (GuardTranslator::void visit(const Transition&))"); }
	void visit(const Observer& /*obj*/) override { throw std::logic_error("Unexpected invocation (GuardTranslator::void visit(const Observer&))"); }

	void visit(const ThreadObserverVariable& /*var*/) override { result = context.observer.threadvar; }
	void visit(const ProgramObserverVariable& /*var*/) override { result = context.observer.adrvar; }
	void visit(const SelfGuardVariable& /*var*/) override { result = context.observer.selfparam; }
	void visit(const ArgumentGuardVariable& var) override {
		auto index = param2index.at(&var.decl);
		result = context.observer.params.at(index);
	}
	void visit(const TrueGuard& /*obj*/) override { result = context.context.bool_val(true); }
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
		z3::expr_vector conjuncts(context.context);
		for (const auto& conj : guard.conjuncts) {
			conj->accept(*this);
			conjuncts.push_back(result);
		}
		result = z3::mk_and(conjuncts);
	}
};


inline z3::expr translate_guard(Context context, const Transition& transition) {
	GuardTranslator translator(context, transition.label);
	transition.guard->accept(translator);
	return translator.result;
}

// SymbolicTransition::SymbolicTransition(SymbolicObserver& observer, const SymbolicState& dst, const Transition& transition) : dst(dst), label(transition.label), kind(transition.kind), guard(translate_guard(observer, transition)) {
// }

SymbolicTransition::SymbolicTransition(const SymbolicState& dst, const Function& label, Transition::Kind kind, z3::expr guard) : dst(dst), label(label), kind(kind), guard(guard.simplify()) {
}

SymbolicState::SymbolicState(const SymbolicObserver& observer, bool is_final, bool is_active) : observer(observer), is_final(is_final), is_active(is_active) {
}

inline std::size_t find_max_params(const SmrObserverStore& store) {
	std::size_t max = 0;
	apply_to_transitions(store, [&max](const Transition& transition) {
		max = std::max(max, transition.label.args.size());
	});
	return max;
}

inline std::set<std::pair<const Function*, Transition::Kind>> collect_transition_infos(const SmrObserverStore& store) {
	std::set<std::pair<const Function*, Transition::Kind>> result;
	apply_to_transitions(store, [&result](const Transition& transition) {
		result.insert({ &transition.label, transition.kind });
	});
	return result;
}

inline std::set<const State*> collect_final_states(const SmrObserverStore& store) {
	std::set<const State*> result;
	apply_to_states(store, [&result](const State& state) {
		if (state.final) {
			result.insert(&state);
		}
	});
	return result;
}

inline std::size_t guess_final_size(const SmrObserverStore& store) {
	std::size_t result = 1;
	apply_to_observers(store, [&result](const Observer& observer) {
		result *= observer.states.size();
	});
	return result;
}

struct HalfWaySymbolicTransition {
	const State& dst;
	const Function& label;
	Transition::Kind kind;
	z3::expr guard;
	
	HalfWaySymbolicTransition(Context context, const Transition& transition) : dst(transition.dst), label(transition.label), kind(transition.kind), guard(translate_guard(context, transition)) {
	}
	HalfWaySymbolicTransition(const State& dst, const Function& label, Transition::Kind kind, z3::expr guard) : dst(dst), label(label), kind(kind), guard(guard) {
	}
};

inline std::map<const State*, std::list<HalfWaySymbolicTransition>> make_complete_transition_map(Context context, const SmrObserverStore& store, const std::set<std::pair<const Function*, Transition::Kind>>& all_symbols) {
	std::map<const State*, std::list<HalfWaySymbolicTransition>> result;
	apply_to_states(store, [&result,&context](const State& state){
		std::list<HalfWaySymbolicTransition>& list = result[&state];
		for (const auto& transition : state.transitions) {
			list.emplace_back(context, *transition);
		}
	});

	for (auto& [state, transitions] : result) {
		for (const auto& [label, kind] : all_symbols) {
			z3::expr_vector remaining(context.context);

			// collect negated guards from all matching transistions
			for (const auto& transition : transitions) {
				if (&transition.label == label && transition.kind == kind) {
					remaining.push_back(!transition.guard);
				}
			}

			// add transition completion (if necessary)
			z3::expr remaining_guard = z3::mk_and(remaining);
			context.solver.push();
			auto check_result = context.solver.check();
			context.solver.pop();
			if (could_be_sat(check_result)) {
				transitions.emplace_back(*state, *label, kind, remaining_guard);
			}
		}
	}

	return result;
}

struct CrossProductMaker {
	const SmrObserverStore& store;
	Context context;
	const std::set<const State*>& active_states;

	std::vector<std::unique_ptr<SymbolicState>> states;
	std::deque<SymbolicState*> worklist;
	std::map<std::vector<const State*>, SymbolicState*> state2symbolic;

	std::set<const State*> final_states;
	std::set<std::pair<const Function*, Transition::Kind>> all_symbols;
	std::map<const State*, std::list<HalfWaySymbolicTransition>> state2transition;


	CrossProductMaker(const SmrObserverStore& store, Context context, const std::set<const State*>& active_states)
	 	: store(store), context(context), active_states(active_states), final_states(collect_final_states(store)), all_symbols(collect_transition_infos(store)), state2transition(make_complete_transition_map(context, store, all_symbols))
	{
		states.reserve(guess_final_size(store));
	}

	SymbolicState* add_state(std::vector<const State*> state) {
		states.push_back(std::make_unique<SymbolicState>(context.observer, has_nonempty_intersection(state, final_states), has_nonempty_intersection(state, active_states)));
		states.back()->origin = state;
		auto insertion = state2symbolic.insert({ std::move(state), states.back().get() });
		assert(insertion.second);
		worklist.push_back(states.back().get());
		return states.back().get();
	}

	SymbolicState* add_or_get_state(std::vector<const State*> state) {
		auto find = state2symbolic.find(state);
		if (find == state2symbolic.end()) {
			return add_state(std::move(state));
		} else {
			return find->second;
		}
	}

	void prepare_initial_states() {
		// find initial states per observer
		std::vector<std::set<const State*>> initial_states;
		apply_to_observers(store, [&initial_states](const Observer& observer) {
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
			add_or_get_state(maker.get_next());
		}
	}

	std::vector<std::set<const HalfWaySymbolicTransition*>> get_transitions_per_state(const SymbolicState& symbolic_state, const Function* label, Transition::Kind kind) {
		assert(!state.origin.empty());
		std::vector<std::set<const HalfWaySymbolicTransition*>> result;
		for (const State* state : symbolic_state.origin) {
			std::set<const HalfWaySymbolicTransition*> transitions;
			for (const HalfWaySymbolicTransition& trans : state2transition.at(state)) {
				if (&trans.label == label && trans.kind == kind) {
					transitions.insert(&trans);
				}
			}
			result.push_back(std::move(transitions));
		}
		return result;
	}

	void handle_symbolicstate(SymbolicState& state) {
		// post image für alle symbole
		for (const auto& [label, kind] : all_symbols) {
			auto transitions_per_state = get_transitions_per_state(state, label, kind);
			CombinationMaker combinator(transitions_per_state);
			assert(combinator.available());
			while (combinator.available()) {
				// compute possible transition in cross product
				std::vector<const HalfWaySymbolicTransition*> combination = combinator.get_next();
				z3::expr_vector guards(context.context);
				std::vector<const State*> post;
				for (const HalfWaySymbolicTransition* transition : combination) {
					guards.push_back(transition->guard);
					post.push_back(&transition->dst);
				}

				// check if new transition is required (guard sat?)
				z3::expr new_guard = z3::mk_and(guards);
				context.solver.push();
				context.solver.add(new_guard);
				auto check_result = context.solver.check();
				context.solver.pop();
				
				if (could_be_sat(check_result)) {
					// add new transition
					SymbolicState& post_state = *add_or_get_state(std::move(post));
					state.transitions.push_back(std::make_unique<SymbolicTransition>(post_state, *label, kind, new_guard));
				}
			}
		}
	}

	void post_process() {
		// ensure that final states cannot reach non-final states
		bool has_final_state = false;
		for (const auto& state : states) {
			if (state->is_final) {
				has_final_state = true;
				for (const auto& transition : state->transitions) {
					conditionally_raise_error<UnsupportedObserverError>(!transition->dst.is_final, "final states must not reach non-final states");
				}
			}
		}
		if (!has_final_state) {
			return;
		}

		// create unique final state, add self-loops
		auto final_state = std::make_unique<SymbolicState>(context.observer, true, false); // TODO: is the final state active?
		for (const auto& [label, kind] : all_symbols) {
			final_state->transitions.push_back(std::make_unique<SymbolicTransition>(*final_state, *label, kind, context.context.bool_val(true)));
		}

		// replace final states with unique one in transitions
		auto make_replacement = [&final_state](const SymbolicTransition& transition) -> std::unique_ptr<SymbolicTransition> {
			return std::make_unique<SymbolicTransition>(*final_state, transition.label, transition.kind, transition.guard);
		};
		for (auto& state : states) {
			for (auto& transition : state->transitions) {
				if (transition->dst.is_final) {
					transition = make_replacement(*transition);
				}
			}
		}

		// remove final states
		std::vector<std::unique_ptr<SymbolicState>> all_states = std::move(states);
		states.clear();
		for (auto& state : all_states) {
			if (!state->is_final) {
				states.push_back(std::move(state));
			}
		}
		states.push_back(std::move(final_state));
	}

	void compute_cross_product() {
		prepare_initial_states();

		while (!worklist.empty()) {
			SymbolicState* current = worklist.front();
			worklist.pop_front();

			handle_symbolicstate(*current);
		}

		post_process();

		// // debug output
		// std::cout << "#states = " << states.size() << std::endl;
		// auto print_sstate = [](const SymbolicState& symbolic_state) {
		// 	std::cout << "{ ";
		// 	bool first = true;
		// 	for (const auto& state : symbolic_state.origin) {
		// 		if (!first) std::cout << ", ";
		// 		first = false;
		// 		std::cout << state->name;
		// 	}
		// 	std::cout << " }";
		// };
		// for (const auto& state : states) {
		// 	std::cout << "++ ";
		// 	if (state->is_final) std::cout << "final ";
		// 	if (state->is_active) std::cout << "active ";
		// 	std::cout << "state: ";
		// 	print_sstate(*state);
		// 	std::cout << std::endl;
		// 	for (const auto& transition : state->transitions) {
		// 		std::cout << "    --[ " << (transition->kind == Transition::INVOCATION ? "enter " : "exit ") << transition->label.name << " ]--> ";
		// 		print_sstate(transition->dst);
		// 		std::cout << "    // " << transition->guard << std::endl;
		// 	}
		// }
	}
};

inline std::vector<std::unique_ptr<SymbolicState>> make_states(const SmrObserverStore& store, Context context) {
	std::set<const State*> active_states;
	for (const auto& state : store.base_observer->states) {
		if (state->initial) {
			active_states.insert(state.get());
		}
	}

	CrossProductMaker maker(store, context, active_states);
	maker.compute_cross_product();
	return std::move(maker.states);
}

SymbolicObserver::SymbolicObserver(const SmrObserverStore& store) : solver(context), threadvar(context.int_const("__THREAD")), adrvar(context.int_const("__ADR")), selfparam(context.int_const("self")) {
	// add param variables to context
	std::size_t max_params = find_max_params(store);
	for (std::size_t index = 0; index < max_params; ++index) {
		std::string name = "param_" + std::to_string(index);
		this->params.push_back(context.int_const(name.c_str()));
	}

	// prepare passing stuff around
	Context my_context(*this, context, solver);

	// compute cross product
	this->states = make_states(store, my_context);

	// compute closures
	for (auto& state : this->states) {
		state->closure = compute_closure(my_context, { state.get() });
	}
}


//
// SymbolicObserver operations
//
inline bool is_equal(const Expression& expression, const Expression& other) {
	const VariableExpression* var_expression = dynamic_cast<const VariableExpression*>(&expression);
	const VariableExpression* var_other = dynamic_cast<const VariableExpression*>(&other);
	if (var_expression && var_other) {
		return &var_expression->decl == &var_other->decl;
	}
	throw std::logic_error("Cannot compare expressions; must be 'VariableExpression'.");
}

inline std::tuple<const Function*, Transition::Kind, z3::expr> prepare(const SymbolicObserver& observer, z3::context& context, const Enter& command, const VariableDeclaration& variable) {
	z3::expr_vector constraints(context);

	// force post for the executing threads
	constraints.push_back(observer.selfparam == observer.threadvar);

	// map command argument equalities to command.decl.args equalities
	for (std::size_t index = 0; index < command.args.size(); ++index) {
		for (std::size_t other = index + 1; other < command.args.size(); ++other) {
			if (is_equal(*command.args.at(index), *command.args.at(other))) {
				constraints.push_back(observer.params.at(index) == observer.params.at(other));
			}
		}
	}

	// map command argument-variable equalities to command.decl.args-adrvar equalities
	auto dummy_var_expression = std::make_unique<VariableExpression>(variable);
	for (std::size_t index = 0; index < command.args.size(); ++index) {
		if (is_equal(*command.args.at(index), *dummy_var_expression)) {
			constraints.push_back(observer.params.at(index) == observer.adrvar);
		}
	}

	// put result together
	return { &command.decl, Transition::INVOCATION, z3::mk_and(constraints) };
}

inline std::tuple<const Function*, Transition::Kind, z3::expr> prepare(const SymbolicObserver& observer, const Exit& command) {
	return { &command.decl, Transition::RESPONSE, observer.selfparam == observer.threadvar };
}

inline std::tuple<const Function*, Transition::Kind, z3::expr> prepare(const SymbolicObserver& observer, z3::context& context, const Command& command, const VariableDeclaration& variable) {
	const Enter* enter = dynamic_cast<const Enter*>(&command);
	if (enter) {
		return prepare(observer, context, *enter, variable);
	}
	const Exit* exit = dynamic_cast<const Exit*>(&command);
	if (exit) {
		return prepare(observer, *exit);
	}
	throw std::logic_error("Cannot compute post image for command; must be 'Enter' or 'Exit'.");
}

inline bool matches(const SymbolicTransition& transition, const Function* label, Transition::Kind kind) {
	return &transition.label == label && transition.kind == kind;
}


SymbolicStateSet prtypes::symbolic_post(const SymbolicState& state, const Command& command, const VariableDeclaration& variable) {
	auto& observer = state.observer;

	const auto& [label, kind, constraint] = prepare(observer, observer.context, command, variable);
	observer.solver.push();
	observer.solver.add(constraint);

	SymbolicStateSet result;
	for (const auto& transition : state.transitions) {
		if (matches(*transition, label, kind) && result.count(&transition->dst) == 0) {
			observer.solver.push();
			observer.solver.add(transition->guard);
			auto check_result = observer.solver.check();
			observer.solver.pop();
			if (could_be_sat(check_result)) {
				result.insert(&transition->dst);
			}
		}
	}

	observer.solver.pop();
	return result;
}

SymbolicStateSet prtypes::symbolic_post(const SymbolicStateSet& set, const Command& command, const VariableDeclaration& variable) {
	SymbolicStateSet result;
	for (const SymbolicState* state : set) {
		auto post = prtypes::symbolic_post(*state, command, variable);
		result.insert(post.begin(), post.end());
	}
	return result;
}

SymbolicStateSet prtypes::symbolic_closure(const SymbolicStateSet& set) {
	SymbolicStateSet result;
	for (const SymbolicState* state : set) {
		result.insert(state->closure.begin(), state->closure.end());
	}
	return result;
}

SymbolicStateSet prtypes::symbolic_closure(const SymbolicState& state) {
	return state.closure;
}
