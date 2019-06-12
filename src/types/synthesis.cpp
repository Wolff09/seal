#include "types/synthesis.hpp"

#include "cola/util.hpp"
#include "cola/util.hpp"
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
		std::string line = "  -[" + std::to_string(elem.first.size()) + "|" + std::to_string(elem.second.size()) + "] ";
		line += print(elem.first);
		line += "|  ";
		line += print(elem.second);
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


std::unique_ptr<Observer> make_blueprint(const Observer& observer) {
	auto result = cola::copy(observer);
	result->negative_specification = false;
	// do not change final states for now (they are used to prune the synthesized types)
	// do it later, after synthesis
	// for (const auto& state : result->states) {
	// 	state->final = false;
	// }
	return result;
}

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
	return is_sat(solver, std::move(expr));
}

TransitionInfo make_transition_info(z3::context& context, z3::solver& solver, const Transition& transition) {
	auto formula = translate(context, *transition.guard);
	bool closure = needs_closure(context, solver, formula);
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
					result[key].emplace_back(state.get(), label, kind, staying, needs_closure(context, solver, staying), is_the_only_one);
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

struct CrossProducer {
	using crossstate_t = std::set<const State*>; // TODO: should be a set
	using worklist_t = std::deque<State*>;

	std::vector<std::reference_wrapper<const Observer>> observers;
	z3::context context;
	z3::solver solver;
	std::set<std::pair<const Function*, Transition::Kind>> symbols;
	std::map<TransitionKey, std::vector<TransitionInfo>> post_map;

	std::unique_ptr<Observer> result;
	State* final_state;
	std::map<crossstate_t, State*> state2cross;
	std::map<const State*, crossstate_t> cross2state;
	GuardCopyTransformer guardguy;

	CrossProducer(std::vector<std::reference_wrapper<const Observer>> observers) : observers(observers), context(), solver(context) {
		add_observers_to_z3(context, solver, observers);
		auto [symbols, post_map] = make_post_map(context, solver, observers);
		this->symbols = std::move(symbols);
		this->post_map = std::move(post_map);

		// TODO: ensure that all observers have the same type and layout of observer variables
		// TODO: ensure that final states can only reach final states
	}

	void init_result() {
		result = std::make_unique<Observer>();

		// add variables
		assert(!observers.empty());
		const Observer& src = observers.front();
		for (const auto& var : src.variables) {
			auto copy = cola::copy(*var);
			result->variables.push_back(std::move(copy));
		}
		for (const Observer& observer : observers) {
			assert(observer.variables.size() == src.variables.size());
			for (std::size_t index = 0; index < observer.variables.size(); ++index) {
				assert(typeid(observer.variables.at(index).get()) == typeid(result->variables.at(index).get()));
				guardguy.add_mapping(*observer.variables.at(index).get(), *result->variables.at(index).get());
			}
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
		for (const auto& elem : state) {
			if (elem->final) {
				is_final = true;
				break;
			}
		}
		if (is_final) {
			return final_state;
		} else {
			auto new_state = std::make_unique<State>(make_name(state), false, is_final);
			State* result_state = new_state.get();
			result->states.push_back(std::move(new_state));
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
		// TODO: ensure that observers have at most one initial state
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

		if (&src == &dst) {
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

	std::unique_ptr<Observer> make_cross_product() {
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

		return std::move(result);
	}
};


struct Synthesizer {
	using stateset_t = std::set<const State*>;
	using synthstate_t = std::pair<stateset_t, stateset_t>;
	using reachset_t = std::set<synthstate_t>;
	using worklist_t = std::deque<synthstate_t>;

	std::unique_ptr<Observer> base_observer; // blueprint without final states
	std::unique_ptr<Observer> impl_observer; // blueprint without final states
	z3::context context;
	z3::solver solver;
	std::set<std::pair<const Function*, Transition::Kind>> symbols;
	std::map<TransitionKey, std::vector<TransitionInfo>> post_map;

	Synthesizer(const Observer& base_observer_, const Observer& impl_observer_)
		: base_observer(make_blueprint(base_observer_)), impl_observer(make_blueprint(impl_observer_)), context(), solver(context)
	{
		add_observers_to_z3(context, solver, { *base_observer, *impl_observer });
		auto [symbols, post_map] = make_post_map(context, solver, { *base_observer, *impl_observer });
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
		// TODO: this relies on internal knowledge of how the base observer looks like. BAD!
		reachset_t reach;
		worklist_t worklist;
		for (const auto& state1 : base_observer->states) {
			if (state1->initial) {
				stateset_t base_state({ state1.get() });
				stateset_t impl_state;
				for (const auto& state2 : impl_observer->states) {
					impl_state.insert(state2.get());
				}
				synthstate_t init = { std::move(base_state), std::move(impl_state) };
				auto insertion = reach.insert(init);
				if (insertion.second) {
					worklist.push_back(init);
				}
			}
		}
		assert(!worklist.empty());
		assert(reach.size() <= worklist.size());
		return { reach, worklist };
	}

	bool is_definitely_final(const stateset_t& set) {
		for (const auto& state : set) {
			if (!state->final) {
				return false;
			}
		}
		return true;
	}

	bool is_definitely_final(const synthstate_t& state) {
		return is_definitely_final(state.first) || is_definitely_final(state.second);
	}

	std::deque<synthstate_t> synthesis_post(const synthstate_t& state) {
		std::deque<synthstate_t> result;

		for (auto [label, kind] : symbols) {
			// compute vector of possibile transitions
			std::vector<std::reference_wrapper<const std::vector<TransitionInfo>>> tinfo;
			for (const auto& stateset : { state.first, state.second }) {
				for (const State* state : stateset) {
					TransitionKey key(state, label, kind);
					tinfo.push_back(post_map.at(key));
				}
			}

			// iterate over possibilities
			for (AllCombinator combinator(tinfo); combinator.available(); combinator.next()) {
				auto combination = combinator.get();
				if (is_combination_enabled(combination)) {
					stateset_t base_post, impl_post;
					for (std::size_t index = 0; index < combination.size(); ++index) {
						const State* next = combination.at(index).get().dst;
						if (index < state.first.size()) {
							base_post.insert(next);
						} else {
							impl_post.insert(next);
						}
					}

					result.push_back({ std::move(base_post), std::move(impl_post) });
					if (is_definitely_final(result.back())) {
						result.pop_back();
					}
				}
			}
		}

		return result;
	}

	reachset_t compute_reachability() {
		auto [reach, worklist] = initialize_synthesis();

		while (!worklist.empty()) {
			synthstate_t synthstate = std::move(worklist.front());
			worklist.pop_front();

			auto post = synthesis_post(synthstate);
			for (auto synthpost : post) {
				auto insertion = reach.insert(synthpost);
				if (insertion.second) {
					worklist.push_back(synthpost);
				}
			}
		}

		return reach;
	}

	stateset_t compute_closure(const stateset_t& set) {
		stateset_t closure;
		for (const State* state : set) {
			for (auto [label, kind] : symbols) {
				for (const TransitionInfo& info : post_map.at({ state, label, kind})) {
					if (info.closure) {
						closure.insert(info.dst);
					}
				}
			}
		}
		return closure;
	}

	synthstate_t compute_closure(const synthstate_t& to_close) {
		return { compute_closure(to_close.first), compute_closure(to_close.second) };
	}

	reachset_t synthesize_guarantees() {
		// get reachabel synthstates
		reachset_t reach = compute_reachability();
		std::cout << "Computed fixed point of size: " << reach.size() << std::endl;
		debug(reach, "REACH");

		// compute their closure
		reachset_t result;
		for (const auto& state : reach) {
			result.insert(compute_closure(state));
		}
		std::cout << "Computed closure of size: " << result.size() << std::endl;
		debug(result, "CLOSURE");
		return result;
	}

	std::string make_name(const synthstate_t& state) {
		std::set<std::string> state_names;
		auto add_names = [&](const stateset_t& set) {
			for (const auto& state : set) {
				state_names.insert(state->name);
			}
		};
		add_names(state.first);
		add_names(state.second);
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
		auto synthesis = synthesize_guarantees();

		std::cout << "Adding guarantees:" << std::endl;
		for (const auto& state : synthesis) {
			std::string name = make_name(state);
			std::cout << "  - " << name << std::endl;
			make_states_final(*base_observer, state.first);
			make_states_final(*impl_observer, state.second);
			auto base_obs = cola::copy(*base_observer);
			auto impl_obs = cola::copy(*impl_observer);
			std::vector<std::unique_ptr<Observer>> obs_vec;
			obs_vec.push_back(std::move(base_obs));
			obs_vec.push_back(std::move(impl_obs));
			guarantee_table.add_guarantee(std::move(obs_vec), std::move(name));
		}
	}
};


void prtypes::populate_guarantee_table_with_synthesized_guarantees(GuaranteeTable& guarantee_table) {
	// TODO: ensure that the observers have the same variables? (done by observer_store?) ==> names might differ... add all names and force them to be equal in the SAT formula?
	// TODO: ensure that the observers are negative specifications?

	for (const auto& impl_observer : guarantee_table.observer_store.impl_observer) {
		std::cout << std::endl << "#Generating cross-product observer..." << std::endl;
		auto cross_product = CrossProducer({ *guarantee_table.observer_store.base_observer, *impl_observer }).make_cross_product();
		std::cout << "#Generated cross-product observer. Number of states: " << cross_product->states.size() << std::endl;

		std::cout << std::endl << "#Generating types for cross-product observer" << std::endl;
		Synthesizer(*guarantee_table.observer_store.base_observer, *impl_observer).add_synthesized_guarantees(guarantee_table);
	}

	throw std::logic_error("not yet implemented");
}
