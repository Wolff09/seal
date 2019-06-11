#include "types/synthesis.hpp"

#include "cola/util.hpp"
#include "cola/util.hpp"
#include "z3++.h"
#include <map>
#include <vector>
#include <deque>

using namespace cola;
using namespace prtypes;


struct TransitionInfo {
	const State* dst;
	const Function* label;
	Transition::Kind kind;
	z3::expr formula;
	TransitionInfo(const State* dst, const Function* label, Transition::Kind kind, z3::expr formula) : dst(dst), label(label), kind(kind), formula(formula) {}
};


std::unique_ptr<Observer> make_blueprint(const Observer& observer) {
	auto result = cola::copy(observer);
	result->negative_specification = false;
	for (const auto& state : result->states) {
		state->final = false;
	}
	return result;
}

template<typename T>
void make_states_final(std::vector<T>& state_collection) {
	for (const T& state : state_collection) {
		state->final = true;
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

void add_observers_to_z3(z3::context& /*context*/, z3::solver& /*solver*/, std::initializer_list<std::reference_wrapper<const Observer>> /*observers*/) {
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


TransitionInfo make_transition_info(z3::context& context, const Transition& transition) {
	auto formula = translate(context, *transition.guard);
	return TransitionInfo(&transition.dst, &transition.label, transition.kind, std::move(formula));
}

std::map<const State*, std::vector<TransitionInfo>> make_post_map(z3::context& context, z3::solver& solver, std::initializer_list<std::reference_wrapper<const Observer>> observers) {
	std::set<std::pair<const Function*, Transition::Kind>> labels;
	for (const Observer& observer : observers) {
		for (const auto& transition : observer.transitions) {
			labels.insert({ &transition->label, transition->kind });
		}
	}

	std::map<const State*, std::vector<TransitionInfo>> result;
	for (const Observer& observer : observers) {
		for (const auto& state : observer.states) {
			for (auto [label, kind] : labels) {
				z3::expr staying = context.bool_val(true);
				bool modified_staying = false;
				for (const auto& transition : observer.transitions) {
					if (&transition->src == state.get() && &transition->label == label && transition->kind == kind) {
						auto info = make_transition_info(context, *transition);
						result[state.get()].push_back(info);
						staying = staying && !info.formula;
						modified_staying = true;
					}
				}
				if (!modified_staying || is_sat(solver, staying)) {
					result[state.get()].emplace_back(state.get(), label, kind, staying);
				}
			}
		}
	}
	return result;
}


struct Synthesizer {
	using stateset_t = std::set<const State*>;
	using synthstate_t = std::pair<stateset_t, stateset_t>;
	using reachset_t = std::set<synthstate_t>;
	using worklist_t = std::deque<synthstate_t>;

	const Observer& base_observer;
	const Observer& impl_observer;
	z3::context context;
	z3::solver solver;
	std::map<const State*, std::vector<TransitionInfo>> post_map;

	Synthesizer(const Observer& base_observer, const Observer& impl_observer)
		: base_observer(base_observer), impl_observer(impl_observer), context(), solver(context)
	{
		add_observers_to_z3(context, solver, { base_observer, impl_observer });
		post_map = make_post_map(context, solver, { base_observer, impl_observer });
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
		for (const auto& state1 : base_observer.states) {
			if (state1->initial) {
				stateset_t base_state({ state1.get() });
				stateset_t impl_state;
				for (const auto& state2 : impl_observer.states) {
					impl_state.insert(state2.get());
				}
				synthstate_t init = { std::move(base_state), std::move(impl_state) };
				reach.insert(init);
				worklist.push_back(init);
			}
		}
		assert(!worklist.empty());
		assert(reach.size() <= worklist.size());
		return { reach, worklist };
	}

	std::deque<synthstate_t> synthesis_post(synthstate_t state) {
		std::cout << "  posting crossproduct state" << std::endl;
		std::deque<synthstate_t> result;

		std::cout << "  computing transitions for state: ";
		std::vector<std::reference_wrapper<const std::vector<TransitionInfo>>> tinfo;
		for (const auto& stateset : { state.first, state.second }) {
			for (const State* state : stateset) {
				tinfo.push_back(post_map.at(state));
				std::cout << tinfo.back().get().size() << ", ";
			}
		}
		std::cout << std::endl;

		// TODO: only do this per (label,kind) combination to avoid mismatching transitions
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
			}
		}
		
		return result;
	}

	reachset_t synthesize_guarantees() {
		std::cout << "Initializing fixpoint iteration" << std::endl;
		auto [reach, worklist] = initialize_synthesis();

		std::cout << "Starting fixpoint with #worklist=" << worklist.size() << std::endl;
		while (!worklist.empty()) {
			const auto& synthstate = worklist.front();

			auto post = synthesis_post(synthstate);
			for (auto synthpost : post) {
				auto insertion = reach.insert(synthpost);
				if (insertion.second) {
					worklist.push_back(synthpost);
				}
			}

			worklist.pop_front();
		}

		return reach;
	}

	void add_synthesized_guarantees(GuaranteeTable& guarantee_table) {
		// base_blueprint(make_blueprint(base_observer)), impl_blueprint(make_blueprint(impl_observer)),
		auto synthesis = synthesize_guarantees();
		for (auto [base_state, impl_state] : synthesis) {
			// TODO: copy the blueprint and make the base_state/impl_state final; then add it to guarantee_table
			// guarantee_table.add_guarantee(std::vector<std::unique_ptr<cola::Observer>> crossproduct_observer, std::string name);
			throw std::logic_error("not yet implemented (add_synthesized_guarantees)");
		}
	}
};


void prtypes::populate_guarantee_table_with_synthesized_guarantees(GuaranteeTable& guarantee_table) {
	// TODO: ensure that the observers have the same variables? (done by observer_store?) ==> names might differ... add all names and force them to be equal in the SAT formula?
	// TODO: ensure that the observers are negative specifications?

	for (const auto& impl_observer : guarantee_table.observer_store.impl_observer) {
		std::cout << "#Generating types for impl_observer" << std::endl;
		Synthesizer(*guarantee_table.observer_store.base_observer, *impl_observer).add_synthesized_guarantees(guarantee_table);
	}

	throw std::logic_error("not yet implemented");
}