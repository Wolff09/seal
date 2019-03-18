#include "types/simulation.hpp"
#include "types/util.hpp"
#include "types/error.hpp"
#include "types/assumption.hpp"
#include "z3++.h"
#include <set>
#include <string>

using namespace cola;
using namespace prtypes;


struct GetObservedAddressVisitor : public ObserverVisitor {
	// TODO: support more general observers with arbitrarily many observered addresses, but only one that is used in frees, and only free lead to accepting states
	const VariableDeclaration* decl;
	void visit(const SelfGuardVariable& /*obj*/) { throw std::logic_error("Unexpected invocation: GetObservedAddressVisitor::visit(const SelfGuardVariable&)"); }
	void visit(const ArgumentGuardVariable& /*obj*/) { throw std::logic_error("Unexpected invocation: GetObservedAddressVisitor::visit(const ArgumentGuardVariable&)"); }
	void visit(const TrueGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: GetObservedAddressVisitor::visit(const TrueGuard&)"); }
	void visit(const ConjunctionGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: GetObservedAddressVisitor::visit(const ConjunctionGuard&)"); }
	void visit(const EqGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: GetObservedAddressVisitor::visit(const EqGuard&)"); }
	void visit(const NeqGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: GetObservedAddressVisitor::visit(const NeqGuard&)"); }
	void visit(const State& /*obj*/) { throw std::logic_error("Unexpected invocation: GetObservedAddressVisitor::visit(const State&)"); }
	void visit(const Transition& /*obj*/) { throw std::logic_error("Unexpected invocation: GetObservedAddressVisitor::visit(const Transition&)"); }
	void visit(const ThreadObserverVariable& /*obj*/) { /* do nothing */ }
	void visit(const ProgramObserverVariable& var) { decl = var.decl.get(); }
	void visit(const Observer& observer) {
		for (const auto& var : observer.variables) {
			var->accept(*this);
		}
	}
	const VariableDeclaration& get_ptr_var() const {
		assert(decl);
		return *decl;
	}
};

struct TranslationUnit {
	z3::context context;
	z3::solver solver;
	std::map<std::string, z3::expr> name2expr;

	TranslationUnit() : solver(context) {}

	const z3::expr& variable(std::string name) {
		auto find = name2expr.find(name);
		if (find == name2expr.end()) {
			// add new variable
			auto insertion = name2expr.insert({ name, context.int_const(name.c_str()) });
			assert(insertion.second);
			return insertion.first->second;
		} else {
			// get existing variable
			return find->second;
		}
	}
};

struct TranslationWithPrefixVisitor : public ObserverVisitor {
	const std::string prefix;
	TranslationUnit& translation;
	z3::expr result;

	TranslationWithPrefixVisitor(std::string prefix, TranslationUnit& unit) : prefix(prefix), translation(unit), result(unit.context.bool_val(true)) {}

	z3::expr get_expr() {
		return result;
	}

	void visit(const ThreadObserverVariable& var) {
		result = translation.variable("oThr__" + var.name);
	}
	void visit(const ProgramObserverVariable& var) {
		result = translation.variable("oPtr__" + var.decl->name);
	}
	void visit(const SelfGuardVariable& /*var*/) {
		result = translation.variable("__self__");
	}
	void visit(const ArgumentGuardVariable& var) {
		result = translation.variable(prefix + var.decl.name);
	}
	void visit(const TrueGuard& /*guard*/) {
		result = translation.context.bool_val(true);
	}

	void mk_equality(const ComparisonGuard& guard) {
		guard.lhs.accept(*this);
		z3::expr lhs = result;
		guard.rhs->accept(*this);
		z3::expr rhs = result;
		result = (lhs == rhs);
	}
	void visit(const EqGuard& guard) {
		mk_equality(guard);
	}
	void visit(const NeqGuard& guard) {
		mk_equality(guard);
		result = !result;
	}
	void visit(const ConjunctionGuard& guard) {
		z3::expr_vector conjuncts(translation.context);
		for (const auto& conj : guard.conjuncts) {
			conj->accept(*this);
			conjuncts.push_back(result);
		}
		result = z3::mk_and(conjuncts);
	}

	void visit(const State& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationWithPrefixVisitor::visit(const State&)"); }
	void visit(const Transition& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationWithPrefixVisitor::visit(const Transition&)"); }
	void visit(const Observer& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationWithPrefixVisitor::visit(const Observer&)"); }
};


z3::expr make_formula(const Guard& guard, std::string param_prefix, TranslationUnit& unit) {
	TranslationWithPrefixVisitor visitor(param_prefix, unit);
	guard.accept(visitor);
	return visitor.get_expr();
}

z3::expr make_relation_formula(const Function& label, const std::string& prefix, const std::string& otherPrefix, TranslationUnit& unit, const SimulationEngine::VariableDeclarationSet& skip) {
	z3::expr_vector conjuncts(unit.context);
	for (const auto& decl : label.args) {
		if (skip.count(*decl) == 0) {
			// TODO: better way to compute names that is guaranteed to be consistent with TranslationWithPrefixVisitor
			auto variable = unit.variable(prefix + decl->name);
			auto other = unit.variable(otherPrefix + decl->name);
			conjuncts.push_back(variable == other);
		}
	}
	return z3::mk_and(conjuncts);
}

struct NoConstraints {
	template<typename... Targs>
	z3::expr operator()(TranslationUnit& unit, const Targs&... /*args*/) {
		return unit.context.bool_val(true);
	}
};

struct EqualFreeable {
	z3::expr operator()(TranslationUnit& unit, const Observer& observer, const Transition& match, std::string prefix, std::string otherPrefix) {
		GetObservedAddressVisitor visitor;
		observer.accept(visitor);
		// TODO: better way to compute names that is guaranteed to be consistent with TranslationWithPrefixVisitor
		auto ovar = unit.variable("oPtr__" + visitor.get_ptr_var().name);
		
		// ensure: var == ovar ==> var' == ovar
		z3::expr_vector conjuncts(unit.context);
		for (const auto& decl : match.label.args) {
			auto variable = unit.variable(prefix + decl->name);
			auto other = unit.variable(otherPrefix + decl->name);
			conjuncts.push_back(z3::implies(variable == ovar, other ==ovar)); // TODO: is this the correct direction
		}
		return z3::mk_and(conjuncts);
	}
};

template<typename AdditionalConstraints>
std::vector<const State*> abstract_post(const Observer& observer, const State& to_post, const Transition& match, const SimulationEngine::VariableDeclarationSet& unrelated) {
	static const std::string PREFIX_MATCH = "arg1__";
	static const std::string PREFIX_TRANS = "arg2__";

	TranslationUnit translation;
	z3::expr match_enc = make_formula(*match.guard, PREFIX_MATCH, translation);
	z3::expr relation = make_relation_formula(match.label, PREFIX_MATCH, PREFIX_TRANS, translation, unrelated);
	z3::expr addition = AdditionalConstraints()(translation, observer, match, PREFIX_MATCH, PREFIX_TRANS);
	translation.solver.add(relation);
	translation.solver.add(addition);
	translation.solver.add(match_enc);

	std::vector<const State*> result;
	bool definitely_has_post = false;
	for (const auto& transition : observer.transitions) {
		if (&transition->src == &to_post && &transition->label == &match.label && transition->kind == match.kind) {
			z3::expr trans_enc = make_formula(*transition->guard, PREFIX_TRANS, translation);

			translation.solver.push();
			translation.solver.add(trans_enc);
			auto check_result = translation.solver.check();
			translation.solver.pop();

			switch (check_result) {
				case z3::unknown:
					throw std::logic_error("SMT solving failed: Z3 was unable to prove SAT/UNSAT, returned 'UNKNOWN'. Cannot recover.");
				case z3::unsat:
					// transition->guard is definitely not enabled
					break;
				case z3::sat:
					// transition->guard may be enabled
					result.push_back(&transition->dst);
					if (!definitely_has_post) {
						translation.solver.push();
						translation.solver.add(!trans_enc);
						auto check_post_result = translation.solver.check();
						translation.solver.pop();
						definitely_has_post |= (check_post_result == z3::unsat);
					}

			}
		}
	}

	if (!definitely_has_post) {
		// only add the state back again if needed (unknown if there is an enabled outgoing transition)
		result.push_back(&to_post);
	}

	return result;
}

std::vector<const State*> abstract_post(const Observer& observer, const State& to_post, const Transition& match) {
	return abstract_post<NoConstraints>(observer, to_post, match, {});
}


bool SimulationEngine::is_safe(const Enter& enter, const std::vector<std::reference_wrapper<const VariableDeclaration>>& params, const VariableDeclarationSet& invalid_params) const {
	// translate invalid parameters to function internal argument declarations
	VariableDeclarationSet invalid_translated;
	assert(enter.decl.args.size() == params.size());
	for (std::size_t i = 0; i < params.size(); i++) {
		if (invalid_params.count(params.at(i)) != 0) {
			invalid_translated.insert(*enter.decl.args.at(i));
		}
	}

	// chech safe
	for (const auto& observer : observers) {
		for (const auto& transition : observer->transitions) {
			if (&transition->label == &enter.decl && transition->kind == Transition::INVOCATION /* TODO: && enabled(transition->guard, params) */) {
				const State& pre_state = transition->src;
				const State& post_state = transition->dst;
				std::vector<const State*> post_pre = abstract_post<EqualFreeable>(*observer, pre_state, *transition, invalid_translated);
				for (const State* to_simulate : post_pre) {
					if (!is_in_simulation_relation(*to_simulate, post_state)) {
						return false;
					}
				}
			}
		}
	}
	return true;
}


bool simulation_holds(const Observer& observer, const SimulationEngine::SimulationRelation& current, const State& to_simulate, const State& simulator) {
	if (&to_simulate == &simulator) {
		auto required = std::make_pair(&to_simulate, &simulator);
		return current.count(required) != 0;
	}
	// handle outgoing transitions of to_simulate
	for (const auto& transition : observer.transitions) {
		if (&transition->src == &to_simulate) {
			auto next_to_simulate = &transition->dst;
			auto simulator_post = abstract_post(observer, simulator, *transition);
			for (const auto& next_simulator : simulator_post) {
				auto required = std::make_pair(next_to_simulate, next_simulator);
				if (current.count(required) == 0) {
					return false;
				}
			}
		}
	}
	// handle outgoing transitions of simulator
	for (const auto& transition : observer.transitions) {
		if (&transition->src == &simulator) {
			auto next_simulator = &transition->dst;
			auto to_simulate_post = abstract_post(observer, to_simulate, *transition);
			for (const auto& next_to_simulate : to_simulate_post) {
				auto required = std::make_pair(next_to_simulate, next_simulator);
				if (!current.count(required)) {
					return false;
				}
			}
		}
	}
	return true;
}

// void debug_simulation_relation(const SimulationEngine::SimulationRelation& sim) {
// 	std::cout << "Simulation Relation: " << std::endl;
// 	for (const auto& [lhs, rhs] : sim) {
// 			std::cout << "  " << lhs->name << "  -  " << rhs->name << std::endl;
// 	}
// }

void SimulationEngine::compute_simulation(const Observer& observer) {
	prtypes::raise_if_assumption_unsatisfied(observer);
	SimulationEngine::SimulationRelation result;

	// start with all-relation, pruned by those that are definitely in a simulation relation
	for (const auto& lhs : observer.states) {
		for (const auto& rhs : observer.states) {
			if (prtypes::implies(rhs->final, lhs->final)) {
				result.insert({ lhs.get(), rhs.get() });
			}
		}
	}

	// // remove non-simulation pairs until fixed point
	// bool removed;
	// do {
	// 	removed = false;
	// 	auto it = result.begin();
	// 	while (it != result.end()) {
	// 		if (!simulation_holds(observer, result, *it->first, *it->second)) {
	// 			removed = true;
	// 			it = result.erase(it); // progresses it
	// 		} else {
	// 			it++;
	// 		}
	// 	}
	// } while (removed);

	// store information
	this->observers.insert(&observer);
	this->all_simulations.insert(result.begin(), result.end());
	// debug_simulation_relation(result);
}

bool SimulationEngine::is_in_simulation_relation(const State& state, const State& other) const {
	return all_simulations.count({ &state, &other }) > 0;
}


struct CallVisitor final : Visitor {
	void visit(const VariableDeclaration& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const VariableDeclaration&)"); }
	void visit(const Expression& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Expression&)"); }
	void visit(const BooleanValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const BooleanValue&)"); }
	void visit(const NullValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const NullValue&)"); }
	void visit(const EmptyValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const EmptyValue&)"); }
	void visit(const MaxValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const MaxValue&)"); }
	void visit(const MinValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const MinValue&)"); }
	void visit(const NDetValue& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const NDetValue&)"); }
	void visit(const VariableExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const VariableExpression&)"); }
	void visit(const NegatedExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const NegatedExpression&)"); }
	void visit(const BinaryExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const BinaryExpression&)"); }
	void visit(const Dereference& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Dereference&)"); }
	void visit(const InvariantExpression& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const InvariantExpression&)"); }
	void visit(const InvariantActive& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const InvariantActive&)"); }
	void visit(const Sequence& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Sequence&)"); }
	void visit(const Scope& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Scope&)"); }
	void visit(const Atomic& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Atomic&)"); }
	void visit(const Choice& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Choice&)"); }
	void visit(const IfThenElse& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const IfThenElse&)"); }
	void visit(const Loop& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Loop&)"); }
	void visit(const While& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const While&)"); }
	void visit(const Skip& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Skip&)"); }
	void visit(const Break& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Break&)"); }
	void visit(const Continue& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Continue&)"); }
	void visit(const Assume& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Assume&)"); }
	void visit(const Assert& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Assert&)"); }
	void visit(const Return& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Return&)"); }
	void visit(const Malloc& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Malloc&)"); }
	void visit(const Assignment& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Assignment&)"); }
	void visit(const Macro& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Macro&)"); }
	void visit(const CompareAndSwap& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const CompareAndSwap&)"); }
	void visit(const Function& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Function&)"); }
	void visit(const Program& /*node*/) override { throw std::logic_error("Unexpected invocation: CallVisitor::visit(const Program&)"); }

	const Function* label = nullptr;
	Transition::Kind kind = Transition::INVOCATION;

	std::pair<const Function&, Transition::Kind> convert(const AstNode& node) {
		node.accept(*this);
		assert(label);
		return { *label, kind };
	}

	void visit(const Enter& node) override {
		this->label = &node.decl;
		this->kind = Transition::INVOCATION;
	}
	void visit(const Exit& node) override {
		this->label = &node.decl;
		this->kind = Transition::RESPONSE;
	}
};

struct GuardMakerVisitor final : public ObserverVisitor {
	std::vector<std::unique_ptr<Guard>> guards;
	bool is_thread_observed;
	bool is_address_observed;
	const VariableDeclaration* decl;
	GuardMakerVisitor(bool is_thread_observed, bool is_address_observed, const VariableDeclaration* decl) : is_thread_observed(is_thread_observed), is_address_observed(is_address_observed), decl(decl) {}

	void visit(const SelfGuardVariable& /*obj*/) { throw std::logic_error("Unexpected invocation: GuardMakerVisitor::visit(const SelfGuardVariable&)"); }
	void visit(const ArgumentGuardVariable& /*obj*/) { throw std::logic_error("Unexpected invocation: GuardMakerVisitor::visit(const ArgumentGuardVariable&)"); }
	void visit(const TrueGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: GuardMakerVisitor::visit(const TrueGuard&)"); }
	void visit(const ConjunctionGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: GuardMakerVisitor::visit(const ConjunctionGuard&)"); }
	void visit(const EqGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: GuardMakerVisitor::visit(const EqGuard&)"); }
	void visit(const NeqGuard& /*obj*/) { throw std::logic_error("Unexpected invocation: GuardMakerVisitor::visit(const NeqGuard&)"); }
	void visit(const State& /*obj*/) { throw std::logic_error("Unexpected invocation: GuardMakerVisitor::visit(const State&)"); }
	void visit(const Transition& /*obj*/) { throw std::logic_error("Unexpected invocation: GuardMakerVisitor::visit(const Transition&)"); }
	void visit(const Observer& observer) {
		for (const auto& var : observer.variables) {
			var->accept(*this);
		}
	}
	void visit(const ThreadObserverVariable& var) {
		if (is_thread_observed) {
			guards.push_back(std::make_unique<EqGuard>(var, std::make_unique<SelfGuardVariable>()));
		} else {
			guards.push_back(std::make_unique<NeqGuard>(var, std::make_unique<SelfGuardVariable>()));
		}
	}
	void visit(const ProgramObserverVariable& var) {
		assert(decl);
		if (is_address_observed) {
			guards.push_back(std::make_unique<EqGuard>(var, std::make_unique<ArgumentGuardVariable>(*decl)));
		} else {
			guards.push_back(std::make_unique<NeqGuard>(var, std::make_unique<ArgumentGuardVariable>(*decl)));
		}
	}
};

bool SimulationEngine::is_repeated_execution_simulating(const std::vector<std::reference_wrapper<const Command>>& events) const {
	auto make_guard = [](const Observer& observer, const Function& label, bool is_address_observed, bool is_thread_observed) -> std::unique_ptr<Guard> {
		const VariableDeclaration* decl = nullptr;
		if (label.args.size() > 0) {
			assert(label.args.size() <= 1);
			assert(label.args.at(0)->type.sort == Sort::PTR);
			decl = label.args.at(0).get();
		}
		GuardMakerVisitor visitor(is_thread_observed, is_address_observed, decl);
		observer.accept(visitor);
		return std::make_unique<ConjunctionGuard>(std::move(visitor.guards));
	};

	auto post_for_sequence = [&](const Observer& observer, std::set<const State*> reach, bool is_address_observed, bool is_thread_observed) -> std::set<const State*> {
		assert(reach.size() > 0);
		const State& dummy_state = **reach.begin();
		for (const Command& cmd : events) {
			auto [label, kind] = CallVisitor().convert(cmd);
			std::set<const State*> post;
			Transition dummy_transition(dummy_state, dummy_state, label, kind, make_guard(observer, label, is_address_observed, is_thread_observed));
			for (const State* state : reach) {
				auto state_post = abstract_post(observer, *state, dummy_transition);
				post.insert(state_post.begin(), state_post.end());
			}
			reach = std::move(post);
		}
		return reach;
	};

	for (const Observer* observer : this->observers) {
		for (const auto& source : observer->states) {
			for (bool is_address_observed : { true, false }) {
				for (bool is_thread_observed : { true, false }) {
					std::set<const State*> reach_once = post_for_sequence(*observer, { source.get() }, is_address_observed, is_thread_observed);
					std::set<const State*> reach_twice = post_for_sequence(*observer, reach_once, is_address_observed, is_thread_observed);
					for (const State* lhs : reach_once) {
						bool simulated = false;
						for (const State* rhs : reach_twice) {
							if (this->is_in_simulation_relation(*lhs, *rhs)) {
								simulated = true;
								break;
							}
						}
						if (!simulated) {
							return false;
						}
					}
				}
			}
		}
	}

	return true;
}
