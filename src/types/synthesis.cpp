#include "types/synthesis.hpp"

#include "cola/util.hpp"
#include "cola/util.hpp"
#include "types/error.hpp"
#include "z3++.h"
#include <map>
#include <vector>
#include <deque>
#include <array>

using namespace cola;
using namespace prtypes;


//begin debug
template<typename T>
void debug(const T& collection, std::string info="") {
	std::cout << "States[" << info << "]: " << std::endl;

	auto print = [&](const auto& set) -> std::string {
		std::set<std::string> strings;
		for (const auto& state : set) {
			strings.insert(state->name);
		}
		std::string result = "";
		for (const auto& string : strings) {
			result += string + ", ";
		}
		return result;
	};

	std::set<std::string> lines;
	for (const auto& elem : collection) {
		std::string line = "  -[" + std::to_string(elem.size()) + "] ";
		line += print(elem);
		lines.insert(line);
	}
	for (const auto& line : lines) {
		std::cout << line << std::endl;
	}
}
//end debug




struct TransitionKey {
	const State* dst;
	const Function* label;
	Transition::Kind kind;
	TransitionKey(const State* dst, const Function* label, Transition::Kind kind) : dst(dst), label(label), kind(kind) {}
	inline bool operator<(const TransitionKey& other) const {
		if (dst < other.dst) return true;
		else if (dst > other.dst) return false;
		if (label < other.label) return true;
		else if (label > other.label) return false;
		return kind < other.kind;
    }
};

struct TransitionInfo {
	const State* dst;
	const Function* label;
	Transition::Kind kind;
	z3::expr formula;
	bool closure;
	const Transition* origin; // null if created on the fly for completion
	bool onlyone = false; // if true means that the source state has no outgoing transitions for [label,kind]
	TransitionInfo(const State* dst, const Function* label, Transition::Kind kind, z3::expr formula, bool closure, const Transition* origin) : dst(dst), label(label), kind(kind), formula(formula), closure(closure), origin(origin) {}
	TransitionInfo(const State* dst, const Function* label, Transition::Kind kind, z3::expr formula, bool closure, bool onlyone) : TransitionInfo(dst, label, kind, formula, closure, nullptr) { this->onlyone = onlyone; }
};


template<typename T>
void make_states_final(Observer& observer, T& state_collection) {
	for (auto& state : observer.states) {
		state->final = state_collection.find(state.get()) != state_collection.end();
	}
}


struct AllCombinator {
	std::vector<std::vector<TransitionInfo>::const_iterator> begin, cur, end;
	bool is_available = true;

	AllCombinator(std::vector<std::reference_wrapper<const std::vector<TransitionInfo>>>& vec) {
		for (const std::vector<TransitionInfo>& elem : vec) {
			begin.push_back(elem.begin());
			cur.push_back(elem.begin());
			end.push_back(elem.end());
			assert(elem.begin() != elem.end());
		}
		assert(begin.size() == cur.size());
		assert(cur.size() == end.size());
	}

	bool available() {
		return is_available;
	}

	void next() {
		assert(is_available);
		for (std::size_t index = 0; index < cur.size(); ++index) {
			cur.at(index)++;

			if (cur.at(index) == end.at(index)) {
				cur.at(index) = begin.at(index);
			} else {
				return;
			}
		}
		is_available = false;
	}

	std::vector<std::reference_wrapper<const TransitionInfo>> get() {
		assert(is_available);
		std::vector<std::reference_wrapper<const TransitionInfo>> result;
		for (const auto& it : cur) {
			result.push_back(*it);
		}
		return result;
	}
};


struct GuardTranslator final : public ObserverVisitor {
	z3::context& context;
	z3::expr result;
	
	GuardTranslator(z3::context& context) : context(context), result(context.bool_val(true)) {}

	void visit(const ThreadObserverVariable& /*var*/) override { result = context.int_const("__THRD"); }
	void visit(const ProgramObserverVariable& /*var*/) override { result = context.int_const("__ADR"); }
	void visit(const SelfGuardVariable& /*var*/) override { result = context.int_const("__SELF"); }
	void visit(const ArgumentGuardVariable& var) override { result = context.int_const(var.decl.name.c_str()); }
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
	void visit(const State& /*obj*/) override { throw std::logic_error("Unexpected invocation of GuardTranslator::visit(const State&)"); }
	void visit(const Transition& /*obj*/) override { throw std::logic_error("Unexpected invocation of GuardTranslator::visit(const Transition&)"); }
	void visit(const Observer& /*obj*/) override { throw std::logic_error("Unexpected invocation of GuardTranslator::visit(const Observer&)"); }
};

void add_observers_to_z3(z3::context& /*context*/, z3::solver& /*solver*/, std::vector<std::reference_wrapper<const Observer>> /*observers*/) {
	// do nothing
}

template<typename T>
z3::expr translate(z3::context& context, const T& obj) {
	GuardTranslator translator(context);
	obj.accept(translator);
	return translator.result;
}

bool is_unsat(z3::solver& solver, z3::expr expr) {
	solver.push();
	solver.add(expr);
	auto check_result = solver.check();
	solver.pop();
	switch (check_result) {
		case z3::unknown:
		case z3::sat:
			return false;
		case z3::unsat:
			return true;
	}
}

bool is_sat(z3::solver& solver, z3::expr expr) {
	return !is_unsat(solver, expr);
}


bool needs_closure(z3::context& context, z3::solver& solver, z3::expr expr) {
	z3::expr check = expr && (context.int_const("__THRD") != context.int_const("__SELF"));
	return is_sat(solver, std::move(check));
}

TransitionInfo make_transition_info(z3::context& context, z3::solver& solver, const Transition& transition) {
	auto formula = translate(context, *transition.guard);
	bool closure = (&transition.src != &transition.dst) && needs_closure(context, solver, formula);
	return TransitionInfo(&transition.dst, &transition.label, transition.kind, std::move(formula), closure, &transition);
}

std::pair<std::set<std::pair<const Function*, Transition::Kind>>, std::map<TransitionKey, std::vector<TransitionInfo>>> make_post_map(z3::context& context, z3::solver& solver, std::vector<std::reference_wrapper<const Observer>> observers) {
	std::set<std::pair<const Function*, Transition::Kind>> labels;
	for (const Observer& observer : observers) {
		for (const auto& transition : observer.transitions) {
			labels.insert({ &transition->label, transition->kind });
		}
	}

	std::map<TransitionKey, std::vector<TransitionInfo>> result;
	for (const Observer& observer : observers) {
		for (const auto& state : observer.states) {
			for (auto [label, kind] : labels) {
				TransitionKey key(state.get(), label, kind);
				z3::expr staying = context.bool_val(true);
				bool is_the_only_one = true;
				for (const auto& transition : observer.transitions) {
					if (&transition->src == state.get() && &transition->label == label && transition->kind == kind) {
						auto info = make_transition_info(context, solver, *transition);
						result[key].push_back(info);
						staying = staying && !info.formula;
						is_the_only_one = false;
					}
				}
				if (is_the_only_one || is_sat(solver, staying)) {
					result[key].emplace_back(state.get(), label, kind, staying, false /* needs_closure(context, solver, staying) */, is_the_only_one);
				}
			}
		}
	}

	return { std::move(labels), std::move(result) };
}


struct GuardCopyTransformer final : public ObserverVisitor {
	std::map<const ObserverVariable*, const ObserverVariable*> var2new;
	std::unique_ptr<GuardVariable> current_guard_variable;
	std::unique_ptr<Guard> current_guard;
	
	GuardCopyTransformer() {}

	void add_mapping(const ObserverVariable& oldvar, const ObserverVariable& newvar) {
		var2new[&oldvar] = &newvar;
	}
	std::unique_ptr<Guard> copy(const Guard& guard) {
		current_guard_variable.reset();
		current_guard.reset();
		guard.accept(*this);
		return std::move(current_guard);
	}

	void visit(const SelfGuardVariable& /*var*/) override {
		current_guard_variable = std::make_unique<SelfGuardVariable>();
	}
	void visit(const ArgumentGuardVariable& var) override {
		current_guard_variable = std::make_unique<ArgumentGuardVariable>(var.decl);
	}
	void visit(const TrueGuard& /*guard*/) override {
		current_guard = std::make_unique<TrueGuard>();
	}
	void visit(const ConjunctionGuard& guard) override {
		std::vector<std::unique_ptr<Guard>> subguards;
		for (const auto& conjunct : guard.conjuncts) {
			conjunct->accept(*this);
			subguards.push_back(std::move(current_guard));
		}
		current_guard = std::make_unique<ConjunctionGuard>(std::move(subguards));
	}
	void visit(const EqGuard& guard) override {
		guard.rhs->accept(*this);
		current_guard = std::make_unique<EqGuard>(*var2new.at(&guard.lhs), std::move(current_guard_variable));
	}
	void visit(const NeqGuard& guard) override {
		guard.rhs->accept(*this);
		current_guard = std::make_unique<NeqGuard>(*var2new.at(&guard.lhs), std::move(current_guard_variable));
	}
	void visit(const ThreadObserverVariable& /*obj*/) override { throw std::logic_error("Unexpected invocation of GuardTranslator::visit(const ThreadObserverVariable&)"); }
	void visit(const ProgramObserverVariable& /*obj*/) override { throw std::logic_error("Unexpected invocation of GuardTranslator::visit(const ProgramObserverVariable&)"); }
	void visit(const State& /*obj*/) override { throw std::logic_error("Unexpected invocation of GuardTranslator::visit(const State&)"); }
	void visit(const Transition& /*obj*/) override { throw std::logic_error("Unexpected invocation of GuardTranslator::visit(const Transition&)"); }
	void visit(const Observer& /*obj*/) override { throw std::logic_error("Unexpected invocation of GuardTranslator::visit(const Observer&)"); }
};

struct ObserverVariableTypeVisitor final : public ObserverVisitor {
	std::size_t result = 0;
	void reset() { result = 0; }
	void visit(const ThreadObserverVariable& /*obj*/) override { result = 1; }
	void visit(const ProgramObserverVariable& var) override { result = var.decl->type.sort == Sort::PTR ? 2 : 3; }
	void visit(const SelfGuardVariable& /*obj*/) override { throw std::logic_error("Unexpected invocation of ObserverVariableTypeVisitor(const SelfGuardVariable&)"); }
	void visit(const ArgumentGuardVariable& /*obj*/) override { throw std::logic_error("Unexpected invocation of ObserverVariableTypeVisitor(const ArgumentGuardVariable&)"); }
	void visit(const TrueGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation of ObserverVariableTypeVisitor(const TrueGuard&)"); }
	void visit(const ConjunctionGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation of ObserverVariableTypeVisitor(const ConjunctionGuard&)"); }
	void visit(const EqGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation of ObserverVariableTypeVisitor(const EqGuard&)"); }
	void visit(const NeqGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation of ObserverVariableTypeVisitor(const NeqGuard&)"); }
	void visit(const State& /*obj*/) override { throw std::logic_error("Unexpected invocation of ObserverVariableTypeVisitor(const State&)"); }
	void visit(const Transition& /*obj*/) override { throw std::logic_error("Unexpected invocation of ObserverVariableTypeVisitor(const Transition&)"); }
	void visit(const Observer& /*obj*/) override { throw std::logic_error("Unexpected invocation of ObserverVariableTypeVisitor(const Observer&)"); }
};

bool do_types_match(const ObserverVariable& var, const ObserverVariable& other) {
	ObserverVariableTypeVisitor visitor;
	var.accept(visitor);
	std::size_t var_type = visitor.result;
	visitor.reset();
	other.accept(visitor);
	std::size_t other_type = visitor.result;
	return var_type == other_type;
}

struct CrossProducer {
	using crossstate_t = std::set<const State*>;
	using worklist_t = std::deque<State*>;

	std::vector<std::reference_wrapper<const Observer>> observers;
	std::set<const State*> active_states;
	z3::context context;
	z3::solver solver;
	std::set<std::pair<const Function*, Transition::Kind>> symbols;
	std::map<TransitionKey, std::vector<TransitionInfo>> post_map;

	std::unique_ptr<Observer> result;
	std::set<const State*> result_active;
	State* final_state;
	std::map<crossstate_t, State*> state2cross;
	std::map<const State*, crossstate_t> cross2state;
	GuardCopyTransformer guardguy;

	CrossProducer(std::vector<std::reference_wrapper<const Observer>> observers, std::set<const State*> active) : observers(observers), active_states(std::move(active)), context(), solver(context) {
		add_observers_to_z3(context, solver, observers);
		auto [symbols, post_map] = make_post_map(context, solver, observers);
		this->symbols = std::move(symbols);
		this->post_map = std::move(post_map);
	}

	void init_result() {
		result = std::make_unique<Observer>();
		result_active.clear();

		// add variables
		assert(!observers.empty());
		const Observer& src = observers.front();
		for (const auto& var : src.variables) {
			auto copy = cola::copy(*var);
			result->variables.push_back(std::move(copy));
		}
		for (const Observer& observer : observers) {
			conditionally_raise_error<TypeSynthesisError>(!observer.negative_specification, "observers are incompatible for cross product (observers must be negative specifications)");
			// check variables
			conditionally_raise_error<TypeSynthesisError>(observer.variables.size() != src.variables.size(), "observers are incompatible for cross product (different number of observer variables)");
			for (std::size_t index = 0; index < observer.variables.size(); ++index) {
				conditionally_raise_error<TypeSynthesisError>(!do_types_match(*observer.variables.at(index), *result->variables.at(index)), "observers are incompatible for cross product (different observer variable layout)");
				guardguy.add_mapping(*observer.variables.at(index).get(), *result->variables.at(index).get());
			}
			// check transitions
			for (const auto& transition : observer.transitions) {
				conditionally_raise_error<TypeSynthesisError>(transition->src.final && !transition->dst.final, "observers are incompatible for cross product (final states must not reach non-final states)");
			}
			// check states
			bool seen_init = false;
			for (const auto& state : observer.states) {
				conditionally_raise_error<TypeSynthesisError>(seen_init && state->initial, "observers are incompatible for cross product (there must not be more than one initial state)");
				seen_init |= state->initial;
			}
			conditionally_raise_error<TypeSynthesisError>(!seen_init, "observers are incompatible for cross product (there must at least one initial state)");
		}

		// add single final state
		auto fstate = std::make_unique<State>("final", false, true);
		final_state = fstate.get();
		result->states.push_back(std::move(fstate));
	}

	std::string make_name(const crossstate_t& set) {
		std::set<std::string> names;
		for (const State* state : set) {
			names.insert(state->name);
		}
		std::stringstream result;
		result << "{ ";
		bool first = true;
		for (const auto& name : names) {
			if (!first) {
				result << ", ";
			} else {
				first = false;
			}
			result << name;
		}
		result << " }";
		return result.str();
	}

	State* make_new_state(const crossstate_t& state) {
		// we assume that there is only one initial state that is handled separately
		bool is_final = false;
		bool is_active = false;
		for (const auto& elem : state) {
			if (is_final || elem->final) {
				is_final = true;
			}
			if (is_active || active_states.count(elem) > 0) {
				is_active = true;
			}
		}
		if (is_final) {
			return final_state;
		} else {
			auto new_state = std::make_unique<State>(make_name(state), false, is_final);
			State* result_state = new_state.get();
			result->states.push_back(std::move(new_state));
			if (is_active) {
				result_active.insert(result_state);
			}
			return result_state;
		}
	}

	std::pair<State*, bool> add(crossstate_t state) {
		assert(result);
		// return {s,b} with: s being the state and b indicating if a new state was added (true => new state added)
		auto find = state2cross.find(state);
		if (find != state2cross.end()) {
			return { find->second, false };
		} else {
			auto new_state = make_new_state(state);
			state2cross[state] = new_state;
			cross2state[new_state] = std::move(state);
			return { new_state, true };
		}
	}

	State* get_initial_state() {
		crossstate_t initial_state;
		for (const Observer& observer : observers) {
			for (const auto& state : observer.states) {
				if (state->initial) {
					initial_state.insert(state.get());
					break;
				}
			}
		}
		auto result = add(initial_state);
		assert(result.second);
		assert(!result.first->final);
		result.first->initial = true;
		return result.first;
	}

	bool is_combination_enabled(const std::vector<std::reference_wrapper<const TransitionInfo>>& vec) {
		assert(vec.size() > 0);
		auto label = vec.at(0).get().label;
		auto kind = vec.at(0).get().kind;

		for (const TransitionInfo& tinfo : vec) {
			if (tinfo.label != label || tinfo.kind != kind) {
				return false;
			}
		}

		z3::expr_vector conjuncts(context);
		for (const TransitionInfo& tinfo : vec) {
			conjuncts.push_back(tinfo.formula);
		}
		z3::expr conjunction = z3::mk_and(conjuncts);
		return is_sat(solver, conjunction);
	}

	std::unique_ptr<Guard> make_guard_for_crossproduct(const Guard& guard) {
		return guardguy.copy(guard);
	}

	void add_transition_for_post(const State& src, const State& dst, const Function& label, Transition::Kind kind, const std::vector<std::reference_wrapper<const TransitionInfo>>& combination) {
		// ensure that every non-onlyone transition guards are pairwise equivalent
		for (auto it1 = combination.begin(); it1 != combination.end(); ++it1) {
			if (it1->get().onlyone) {
				continue;
			}
			for (auto it2 = it1 + 1; it2 != combination.end(); ++it2) {
				if (it2->get().onlyone) {
					continue;
				}
				if (is_sat(solver, !(it1->get().formula == it2->get().formula))) {
					throw std::logic_error("Cannot construct cross-product observer. Given observers are incompatible (due to their synchronization).");
				}
			}
		}

		if (&src == &dst || &src == final_state) {
			return; // we do not need the transition
		}

		for (const TransitionInfo& info : combination) {
			if (info.origin) {
				auto new_transition = std::make_unique<Transition>(src, dst, label, kind, make_guard_for_crossproduct(*info.origin->guard));
				result->transitions.push_back(std::move(new_transition));
				return;
			}
		}
		throw std::logic_error("Cannot construct cross-product observer. Could not find a guard to create transition (this is definitely weird).");
	}

	std::set<State*> crossproduct_post(const State& to_post) {
		std::set<State*> result;
		const crossstate_t& state_to_post = cross2state.at(&to_post);

		for (auto [label, kind] : symbols) {
			std::vector<std::reference_wrapper<const std::vector<TransitionInfo>>> tinfo;
			for (const State* state : state_to_post) {
				TransitionKey key(state, label, kind);
				tinfo.push_back(post_map.at(key));
			}

			for (AllCombinator combinator(tinfo); combinator.available(); combinator.next()) {
				auto combination = combinator.get();
				if (is_combination_enabled(combination)) {
					crossstate_t post;
					for (const TransitionInfo& info : combination) {
						post.insert(info.dst);
					}

					auto insertion = add(std::move(post));
					add_transition_for_post(to_post, *insertion.first, *label, kind, combination);

					if (insertion.second) {
						result.insert(insertion.first);
					}
				}
			}
		}

		return result;
	}

	std::pair<std::unique_ptr<Observer>, std::set<const State*>> make_cross_product() {
		init_result();
		worklist_t worklist = { get_initial_state() };

		while (!worklist.empty()) {
			State* current = worklist.front();
			worklist.pop_front();

			auto post_image = crossproduct_post(*current); // returns only new states
			for (const auto& post : post_image) {
				worklist.push_back(post);
			}
		}

		return { std::move(result), std::move(result_active) };
	}
};

struct Synthesizer {
	using stateset_t = std::set<const State*>;
	using synthstate_t = stateset_t;
	struct synthstate_compare {
	    bool operator() (const synthstate_t& lhs, const synthstate_t& rhs) const {
	    	if (lhs.size() < rhs.size()) {
	    		return true;
	    	} else if (lhs.size() > rhs.size()) {
	    		return false;
	    	} else {
	    		return lhs < rhs;
	    	}
	    }
	};
	using reachset_t = std::set<synthstate_t, synthstate_compare>;
	using worklist_t = std::deque<synthstate_t>;

	std::unique_ptr<Observer> blueprint;
	const stateset_t active_states;
	z3::context context;
	z3::solver solver;
	std::set<std::pair<const Function*, Transition::Kind>> symbols;
	std::map<TransitionKey, std::vector<TransitionInfo>> post_map;

	Synthesizer(std::unique_ptr<Observer> observer, stateset_t active)
		: blueprint(std::move(observer)), active_states(std::move(active)), context(), solver(context)
	{
		blueprint->negative_specification = false;
		add_observers_to_z3(context, solver, { *blueprint });
		auto [symbols, post_map] = make_post_map(context, solver, { *blueprint });
		this->symbols = std::move(symbols);
		this->post_map = std::move(post_map);
	}

	bool is_combination_enabled(const std::vector<std::reference_wrapper<const TransitionInfo>>& vec) {
		assert(vec.size() > 0);
		auto label = vec.at(0).get().label;
		auto kind = vec.at(0).get().kind;

		for (const TransitionInfo& tinfo : vec) {
			if (tinfo.label != label || tinfo.kind != kind) {
				return false;
			}
		}

		z3::expr_vector conjuncts(context);
		for (const TransitionInfo& tinfo : vec) {
			conjuncts.push_back(tinfo.formula);
		}
		z3::expr conjunction = z3::mk_and(conjuncts);
		return is_sat(solver, conjunction);
	}

	std::pair<reachset_t, worklist_t> initialize_synthesis() {
		synthstate_t active(active_states);
		synthstate_t all;
		for (const auto& state : blueprint->states) {
			all.insert(state.get());
		}

		reachset_t reach = { all, active };
		worklist_t worklist = { all, active };
		return { std::move(reach), std::move(worklist) };
	}

	template<typename T>
	inline void synthesis_post_cmd(T& container, const synthstate_t& to_post, const Function* label, Transition::Kind kind) {
		assert(label);
		std::deque<synthstate_t> result;

		// compute vector of possibile transitions
		std::vector<std::reference_wrapper<const std::vector<TransitionInfo>>> tinfo;
		for (const State* state : to_post) {
			TransitionKey key(state, label, kind);
			tinfo.push_back(post_map.at(key));
		}

		// iterate over possibilities
		for (AllCombinator combinator(tinfo); combinator.available(); combinator.next()) {
			auto combination = combinator.get();
			if (is_combination_enabled(combination)) {
				synthstate_t post;
				for (const TransitionInfo& info : combination) {
					post.insert(info.dst);
				}

				container.push_back(std::move(post));
			}
		}
	}

	std::deque<synthstate_t> synthesis_post_cmd(const synthstate_t& to_post, const Function* label, Transition::Kind kind) {
		std::deque<synthstate_t> result;
		synthesis_post_cmd(result, to_post, label, kind);
		return result;
	}

	std::deque<synthstate_t> synthesis_post(const synthstate_t& to_post) {
		std::deque<synthstate_t> result;
		for (auto [label, kind] : symbols) {
			synthesis_post_cmd(result, to_post, label, kind);
		}
		return result;
	}

	synthstate_t compute_closure(const synthstate_t& set) {
		synthstate_t closure(set);
		bool updated;
		do {
			updated = false;
			for (const State* state : closure) {
				for (auto [label, kind] : symbols) {
					for (const TransitionInfo& info : post_map.at({ state, label, kind})) {
						if (info.closure) {
							auto insertion = closure.insert(info.dst);
							if (insertion.second) {
								updated = true;
							}
						}
					}
				}
			}
		} while (updated);
		return closure;
	}

	reachset_t compute_reachability() {
		auto [reach, worklist] = initialize_synthesis();

		while (!worklist.empty()) {
			synthstate_t synthstate = std::move(worklist.front());
			worklist.pop_front();

			auto post = synthesis_post(synthstate);
			for (auto& synthpost : post) {
				auto insertion = reach.insert(synthpost);
				if (insertion.second) {
					worklist.push_back(std::move(synthpost));
				}
			}
		}

		return reach;
	}

	reachset_t synthesize_guarantees() {
		// get reachabel synthstates
		reachset_t reach = compute_reachability();
		// debug(reach, "REACH");

		// compute their closure
		// we do closure as a last step since the post image will contain the closure
		reachset_t result;
		for (const auto& state : reach) {
			result.insert(compute_closure(state));
		}
		// debug(result, "CLOSURE");
		return result;
	}

	std::string make_name(const synthstate_t& set) {
		std::set<std::string> state_names;
		for (const auto& state : set) {
			state_names.insert(state->name);
		}
		std::stringstream result;
		result << "{ ";
		bool first = true;
		for (const auto& name : state_names) {
			if (!first) {
				result << ", ";
			}
			first = false;
			result << name;
		}
		result << " }";
		return result.str();
	}

	void add_synthesized_guarantees(GuaranteeTable& guarantee_table) {
		const auto synthesis = synthesize_guarantees();

		// std::cout << "Adding guarantees:" << std::endl;
		std::map<const Guarantee*, const synthstate_t*> guarantee2states;
		for (const auto& state : synthesis) {
			std::string name = make_name(state);
			// std::cout << "  - " << name << std::endl;
			make_states_final(*blueprint, state);
			auto obs = cola::copy(*blueprint);
			auto guarantees = guarantee_table.add_guarantee(std::move(obs), std::move(name));
			for (const Guarantee& guarantee : guarantees) {
				guarantee2states.insert({ &guarantee, &state });
			}
		}

		auto is_include = [](const synthstate_t& smaller, const synthstate_t& bigger, bool proper=false) {
			if (smaller.size() > bigger.size()) return false;
			if (proper && smaller.size() == bigger.size()) return false;
			for (const auto& state : smaller) {
				if (bigger.count(state) == 0) {
					return false;
				}
			}
			return true;
		};

		// prune guarantees that are smaller than some valid guarantee
		// std::cout << std::endl << "Pruning guarantees:" << std::endl;
		std::set<const Guarantee*> keep;
		for (const auto& pair : guarantee2states) {
			keep.insert(pair.first);
		}
		for (const Guarantee* guarantee : keep) {
			if (guarantee->entails_validity && guarantee != &guarantee_table.active_guarantee() && guarantee != &guarantee_table.local_guarantee()) {
				// std::cout << "Found valid guarantee: " << guarantee->name << std::endl;
				for (const Guarantee* other : keep) {
					if (!other->entails_validity && is_include(*guarantee2states.at(other), *guarantee2states.at(guarantee), true)) {
						// std::cout << "   -> pruning: " << other->name << std::endl;
						keep.erase(other);
					}
				}
			}
		}
		std::vector<std::unique_ptr<Guarantee>> all_guarantees = std::move(guarantee_table.all_guarantees);
		guarantee_table.all_guarantees.clear();
		for (auto& guarantee : all_guarantees) {
			if (keep.count(guarantee.get()) != 0) {
				guarantee_table.all_guarantees.push_back(std::move(guarantee));
			} else {
				guarantee2states.erase(guarantee.get());
			}
		}

		// std::cout << "Adding inclusion relations:" << std::endl;
		for (const auto& pair : guarantee2states) {
			GuaranteeSet included_in;
			for (const auto& other_pair : guarantee2states) {
				if (is_include(*pair.second, *other_pair.second)) {
					included_in.insert(*other_pair.first);
				}
			}
			// std::cout << "inclusion for: " << pair.first->name << " = " << included_in.size() << std::endl;
			guarantee_table.inclusion_map[*pair.first] = std::move(included_in);
		}
	}
};


void prtypes::populate_guarantee_table_with_synthesized_guarantees(GuaranteeTable& guarantee_table) {
	const Observer& base_observer = *guarantee_table.observer_store.base_observer;
	std::set<const State*> active;
	for (const auto& state : base_observer.states) {
		if (state->initial) {
			active.insert(state.get()); // this relies on knowledge about how the base observer looks like. BAD!
		}
	}

	for (const auto& impl_observer : guarantee_table.observer_store.impl_observer) {
		// std::cout << std::endl << "#Generating cross-product observer..." << std::endl;
		auto [cross_product, active_states] = CrossProducer({ base_observer, *impl_observer }, active).make_cross_product();
		// std::cout << "#Generated cross-product observer. Number of states (active): " << cross_product->states.size() << " (" << active_states.size() << ")" << std::endl;
		// cola::print(*cross_product, std::cout);

		Synthesizer(std::move(cross_product), std::move(active_states)).add_synthesized_guarantees(guarantee_table);
	}

	// sort guarantee table such that valid guarantees appear first
	std::vector<std::unique_ptr<Guarantee>> valid;
	std::vector<std::unique_ptr<Guarantee>> nonvalid;
	for (auto& guarantee : guarantee_table.all_guarantees) {
		bool is_valid = guarantee->entails_validity || guarantee.get() == &guarantee_table.active_guarantee() || guarantee.get() == &guarantee_table.local_guarantee();
		(is_valid ? valid : nonvalid).push_back(std::move(guarantee));
	}
	guarantee_table.all_guarantees.clear();
	std::array<std::reference_wrapper<decltype(valid)>, 2> vectors = { valid, nonvalid };
	for (decltype(valid)& container : vectors) {
		for (auto& guarantee : container) {
			guarantee_table.all_guarantees.push_back(std::move(guarantee));
		}
	}
}
