#include "types/simulation.hpp"
#include "types/util.hpp"
#include "z3++.h"
#include <set>
#include <string>

using namespace cola;
using namespace prtypes;


// struct TranslationVisitor : public ObserverVisitor {
// 	std::set<std::string> names;
// 	std::string result = "";

// 	void visit(const ThreadObserverVariable& var) {
// 		result = "$" + var.name;
// 		names.insert(result);
// 	}
// 	void visit(const ProgramObserverVariable& var) {
// 		result = "&" + var.decl->name;
// 		names.insert(result);
// 	}
// 	void visit(const SelfGuardVariable& /*var*/) {
// 		result = "@self";
// 		names.insert(result);
// 	}
// 	void visit(const ArgumentGuardVariable& var) {
// 		result = "#" + var.decl.name;
// 	}
// 	void visit(const TrueGuard& /*guard*/) {
// 		result = "true";
// 	}

// 	void handle_comparison(const ComparisonGuard& guard, std::string cmp_op) {
// 		result = "";
// 		guard.lhs.accept(*this);
// 		std::string lhs = result;
// 		result = "";
// 		guard.rhs->accept(*this);
// 		std::string rhs = result;
// 		result = "(" + cmp_op + " " + lhs + " " + rhs + ")";
// 	}
// 	void visit(const EqGuard& guard) {
// 		handle_comparison(guard, "=");
// 	}
// 	void visit(const NeqGuard& guard) {
// 		handle_comparison(guard, "!=");
// 	}
// 	void visit(const ConjunctionGuard& guard) {
// 		std::vector<std::string> conjuncts;
// 		result = "";
// 		for (const auto& conj : guard.conjuncts) {
// 			conj->accept(*this);
// 			conjuncts.push_back(result);
// 		}
// 		result = "(and ";
// 		for (const auto& conj : conjuncts) {
// 			result = result + " " + conj;
// 		}
// 		result = ")";
// 	}

// 	void visit(const State& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationVisitor::visit(const State&)"); }
// 	void visit(const Transition& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationVisitor::visit(const Transition&)"); }
// 	void visit(const Observer& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationVisitor::visit(const Observer&)"); }
// };

// bool is_definitely_disabled(const Guard& to_enable, const Guard& premise) {
// 	TranslationVisitor visitor;

// 	// translate expressions
// 	to_enable.accept(visitor);
// 	std::string to_enable_enc = visitor.result;
// 	premise.accept(visitor);
// 	std::string premise_enc = visitor.result;

// 	// create task: 1. variable declarations
// 	std::string task = "";
// 	for (auto name : visitor.names) {
// 		task = task + "(declare-const " + name + " Int) ";
// 	}

// 	// create task: 2. SAT challenge
// 	task = task + "(assert (and " + to_enable_enc + " " + premise_enc + ")) (check sat)";
// 	throw std::logic_error("Check SMTlib encoding: " + task);

// 	// solve
// 	z3::context context;
// 	auto parse_result = context.parse_string(task.c_str());
// 	z3::solver solver(context);
// 	auto result = solver.check();

// 	switch (result) {
// 		case z3::sat: return false;
// 		case z3::unsat: return true;
// 		case z3::unknown: return false;
// 	}
// }

// std::vector<const State*> abstract_post(const Observer& observer, const Transition& match, const State& state) {
// 	std::vector<const State*> result;
// 	for (const auto& transition : observer.transitions) {
// 		if (&transition->src == &state && &transition->label == &match.label && !is_definitely_disabled(*transition->guard, *match.guard)) {
// 			result.push_back(&transition->dst);
// 		}
// 	}
// 	result.push_back(&state); // TODO: implement a check and add 'state' conditionally // TODO: needed? the simulation relation should not be affected!?
// 	return result;
// }

// bool simulation_holds(const Observer& observer, SimulationEngine::SimulationRelation current, const State& to_simulate, const State& simulator) {
// 	for (const auto& transition : observer.transitions) {
// 		if (&transition->src == &to_simulate) {
// 			auto next_to_simulate = &transition->dst;
// 			auto simulator_post = abstract_post(observer, *transition, simulator);
// 			for (const auto& next_simulator : simulator_post) {
// 				auto required = std::make_pair(next_to_simulate, next_simulator);
// 				if (!current.count(required)) {
// 					return false;
// 				}
// 			}
// 		}
// 	}
// 	return true;
// }

// void SimulationEngine::compute_simulation(const Observer& observer) {
// 	SimulationEngine::SimulationRelation result;

// 	// start with all-relation, pruned by those that are definitely in a simulation relation
// 	for (const auto& lhs : observer.states) {
// 		for (const auto& rhs : observer.states) {
// 			if (prtypes::implies(rhs->final, lhs->final)) {
// 				result.insert({ lhs.get(), rhs.get() });
// 			}
// 		}
// 	}

// 	// remove non-simulation pairs until fixed point
// 	bool removed;
// 	do {
// 		removed = false;
// 		auto it = result.begin();
// 		while (it != result.end()) {
// 			if (!simulation_holds(observer, result, *it->first, *it->second)) {
// 				removed = true;
// 				it = result.erase(it); // progresses it
// 			} else {
// 				it++;
// 			}
// 		}
// 	} while (removed);

// 	// store information
// 	this->observers.insert(&observer);
// 	this->all_simulations.insert(result.begin(), result.end());
// }

// bool SimulationEngine::is_in_simulation_relation(const State& state, const State& other) const {
// 	return all_simulations.count({ &state, &other }) > 0;
// }



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

// bool is_transition_definitely_enabled(const TranslationUnit& relation, const TranslationUnit& match, const TranslationUnit& transition) {
// 	// construct implication formula
// 	auto implication = TranslationUnit::merge(match, transition);
// 	implication.assertions.clear();
// 	assert(match.assertions.size() == 1);
// 	assert(transition.assertions.size() == 1);
// 	std::string impl = "(not (=> " + match.assertions.at(0) + " " + transition.assertions.at(0) + "))";
// 	implication.assertions = { impl };

// 	// check for the implication being a tautology (iff the negation is unsat)
// 	auto to_check = TranslationUnit::merge(relation, implication);
// 	return check_formula(to_check.to_string()) == z3::unsat;
// }

std::vector<const State*> abstract_post(const Observer& observer, const State& to_post, const Transition& match, const SimulationEngine::VariableDeclarationSet& unrelated={}) {
	static const std::string PREFIX_MATCH = "arg1__";
	static const std::string PREFIX_TRANS = "arg2__";

	TranslationUnit translation;
	z3::expr match_enc = make_formula(*match.guard, PREFIX_MATCH, translation);
	z3::expr relation = make_relation_formula(match.label, PREFIX_MATCH, PREFIX_TRANS, translation, unrelated);
	translation.solver.add(relation);
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
				std::vector<const State*> post_pre = abstract_post(*observer, pre_state, *transition, invalid_translated);
				for (const State* to_simulate : post_pre) {
					// std::cout << "Checking: " << pre_state.name << "  --  " << post_state.name << "  ||  " << to_simulate->name << std::endl;
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

void debug_simulation_relation(const SimulationEngine::SimulationRelation& sim) {
	std::cout << "Simulation Relation: " << std::endl;
	for (const auto& [lhs, rhs] : sim) {
			std::cout << "  " << lhs->name << "  -  " << rhs->name << std::endl;
	}
}

void SimulationEngine::compute_simulation(const Observer& observer) {
	SimulationEngine::SimulationRelation result;

	// start with all-relation, pruned by those that are definitely in a simulation relation
	for (const auto& lhs : observer.states) {
		for (const auto& rhs : observer.states) {
			if (prtypes::implies(rhs->final, lhs->final)) {
				result.insert({ lhs.get(), rhs.get() });
			}
		}
	}

	// remove non-simulation pairs until fixed point
	bool removed;
	do {
		removed = false;
		auto it = result.begin();
		while (it != result.end()) {
			if (!simulation_holds(observer, result, *it->first, *it->second)) {
				removed = true;
				it = result.erase(it); // progresses it
			} else {
				it++;
			}
		}
	} while (removed);

	// store information
	this->observers.insert(&observer);
	this->all_simulations.insert(result.begin(), result.end());
	debug_simulation_relation(result);
}

bool SimulationEngine::is_in_simulation_relation(const State& state, const State& other) const {
	return all_simulations.count({ &state, &other }) > 0;
}
