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
	std::set<std::string> names;
	std::vector<std::string> assertions;

	template<typename... Targs>
	static TranslationUnit merge(const Targs&... units) {
		TranslationUnit result;
		for (const auto& unit : { units... }) {
			result.names.insert(unit.names.begin(), unit.names.end());
			std::copy(unit.assertions.begin(), unit.assertions.end(), std::back_inserter(result.assertions));
		}
		return result;
	}

	std::string to_string() const {
		static const std::string BREAK = "\n";
		// static const std::string BREAK = " ";
		std::string result;
		for (auto& name : this->names) {
			result += "(declare-const " + name + " Int)" + BREAK;
		}
		result += BREAK;
		for (auto& assertion : this->assertions) {
			result += "(assert " + assertion + ")" + BREAK;
		}
		result += BREAK + "(check-sat)";
		return result;
	}
};

struct TranslationVisitorWithPrefix : public ObserverVisitor {
	const std::string prefix;
	TranslationUnit translation;
	std::set<std::string> names;
	std::string result = "";

	TranslationVisitorWithPrefix(std::string prefix) : prefix(prefix) {}

	TranslationUnit get_result() {
		translation.assertions.push_back(result);
		return translation;
	}

	void visit(const ThreadObserverVariable& var) {
		result = "oThr__" + var.name;
		translation.names.insert(result);
	}
	void visit(const ProgramObserverVariable& var) {
		result = "oPtr__" + var.decl->name;
		translation.names.insert(result);
	}
	void visit(const SelfGuardVariable& /*var*/) {
		result = "__self__";
		translation.names.insert(result);
	}
	void visit(const ArgumentGuardVariable& var) {
		// result = prefix + "#" + var.decl.name;
		result = prefix + var.decl.name;
	}
	void visit(const TrueGuard& /*guard*/) {
		result = "true";
	}

	void handle_comparison(const ComparisonGuard& guard, std::string cmp_op) {
		result = "";
		guard.lhs.accept(*this);
		std::string lhs = result;
		result = "";
		guard.rhs->accept(*this);
		std::string rhs = result;
		result = "(" + cmp_op + " " + lhs + " " + rhs + ")";
	}
	void visit(const EqGuard& guard) {
		handle_comparison(guard, "=");
	}
	void visit(const NeqGuard& guard) {
		handle_comparison(guard, "=");
		result = "(not " + result + ")";
	}
	void visit(const ConjunctionGuard& guard) {
		std::vector<std::string> conjuncts;
		result = "";
		for (const auto& conj : guard.conjuncts) {
			conj->accept(*this);
			conjuncts.push_back(result);
		}
		result = "(and ";
		assert(conjuncts.size() > 0);
		for (const auto& conj : conjuncts) {
			result += " " + conj;
		}
		result += ")";
	}

	void visit(const State& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationVisitorWithPrefix::visit(const State&)"); }
	void visit(const Transition& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationVisitorWithPrefix::visit(const Transition&)"); }
	void visit(const Observer& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationVisitorWithPrefix::visit(const Observer&)"); }
};


TranslationUnit make_formula(const Guard& guard, std::string param_prefix) {
	TranslationVisitorWithPrefix visitor(param_prefix);
	guard.accept(visitor);
	return visitor.get_result();
}

TranslationUnit make_relation_formula(const Function& label, const std::string& prefix, const std::string& otherPrefix, const SimulationEngine::VariableDeclarationSet& skip) {
	TranslationUnit result;
	for (const auto& decl : label.args) {
		if (skip.count(*decl) == 0) {
			// TODO: better way to compute names that is guaranteed to be consistent with TranslationVisitorWithPrefix
			std::string var = prefix + decl->name;
			std::string otherVar = otherPrefix + decl->name;
			result.names.insert(var);
			result.names.insert(otherVar);
			result.assertions.push_back("(= " + var + " " + otherVar + ")");
		}
	}
	return result;
}

z3::check_result check_formula(std::string formula) {
	z3::context context;
	z3::solver solver(context);
	solver.from_string(formula.c_str());
	return solver.check();
}

bool is_transition_definitely_enabled(const TranslationUnit& relation, const TranslationUnit& match, const TranslationUnit& transition) {
	// construct implication formula
	auto implication = TranslationUnit::merge(match, transition);
	implication.assertions.clear();
	assert(match.assertions.size() == 1);
	assert(transition.assertions.size() == 1);
	std::string impl = "(not (=> " + match.assertions.at(0) + " " + transition.assertions.at(0) + "))";
	implication.assertions = { impl };

	// check for the implication being a tautology (iff the negation is unsat)
	auto to_check = TranslationUnit::merge(relation, implication);
	return check_formula(to_check.to_string()) == z3::unsat;
}

std::vector<const State*> abstract_post(const Observer& observer, const State& to_post, const Transition& match, const SimulationEngine::VariableDeclarationSet& unrelated={}) {
	static const std::string PREFIX_MATCH = "arg1__";
	static const std::string PREFIX_TRANS = "arg2__";

	auto match_enc = make_formula(*match.guard, PREFIX_MATCH);
	auto relation = make_relation_formula(match.label, PREFIX_MATCH, PREFIX_TRANS, unrelated);

	std::vector<const State*> result;
	bool definitely_has_post = false;
	for (const auto& transition : observer.transitions) {
		if (&transition->src == &to_post && &transition->label == &match.label && transition->kind == match.kind) {
			auto trans_enc = make_formula(*transition->guard, PREFIX_TRANS);
			auto formula = TranslationUnit::merge(relation, match_enc, trans_enc).to_string();
			// std::cout << "Z3 string: " << formula << std::endl;

			// TODO: use push/pop to avoid repeated construction of z3::context/z3::solver objects
			// One could do the following:
			//  - Create a context and solver per invocation of this function.
			//  - add the decl/assertions from relation to the solver
			//  - add the decl/assertions from match_enc to the solver
			//  - push
			//  - add the decl/assertions from trans_enc to the solver
			//  - solve (check sat)
			//  - pop
			//  - push
			//  - add the decl from trans_enc to the solver
			//  - add the negated assertion from trans_enc to the solver
			//  - solve (check unsat)
			//  - pop
			// Note:
			//  - one has to be careful to avoid duplicate variable declarations
			//  - one should add all possible variables (computable from observer and match.label.args) in the beginning
			//  - one should use the visitors to build a z3::expr directly (no string detour)

			auto check_result = check_formula(formula);
			switch (check_result) {
				case z3::sat:
					// transition->guard may be enabled
					result.push_back(&transition->dst);
					definitely_has_post = definitely_has_post || is_transition_definitely_enabled(relation, match_enc, trans_enc);
					break;
				case z3::unsat:
					// transition->guard is definitely not enabled
					break;
				case z3::unknown:
					throw std::logic_error("SMT solving failed: Z3 was unable to prove SAT/UNSAT, returned 'UNKNOWN'. Cannot recover.");
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

// void debug_sim_rel(const SimulationEngine::SimulationRelation& sim) {
// 	std::cout << "Simulation Relation: " << std::endl;
// 	for (const auto& [lhs, rhs] : sim) {
// 			std::cout << "  " << lhs->name << "  -  " << rhs->name << std::endl;
// 	}
// }

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
	// debug_sim_rel(result);
}

bool SimulationEngine::is_in_simulation_relation(const State& state, const State& other) const {
	return all_simulations.count({ &state, &other }) > 0;
}
