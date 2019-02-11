#include "types/simulation.hpp"
#include "types/util.hpp"
#include "z3++.h"
#include <set>
#include <string>

using namespace cola;
using namespace prtypes;


struct TranslationVisitor : public ObserverVisitor {
	std::set<std::string> names;
	std::string result = "";

	void visit(const ThreadObserverVariable& var) {
		result = "$" + var.name;
		names.insert(result);
	}
	void visit(const ProgramObserverVariable& var) {
		result = "&" + var.decl->name;
		names.insert(result);
	}
	void visit(const SelfGuardVariable& /*var*/) {
		result = "@self";
		names.insert(result);
	}
	void visit(const ArgumentGuardVariable& var) {
		result = "#" + var.decl.name;
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
		handle_comparison(guard, "!=");
	}
	void visit(const ConjunctionGuard& guard) {
		std::vector<std::string> conjuncts;
		result = "";
		for (const auto& conj : guard.conjuncts) {
			conj->accept(*this);
			conjuncts.push_back(result);
		}
		result = "(and ";
		for (const auto& conj : conjuncts) {
			result = result + " " + conj;
		}
		result = ")";
	}

	void visit(const State& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationVisitor::visit(const State&)"); }
	void visit(const Transition& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationVisitor::visit(const Transition&)"); }
	void visit(const Observer& /*obj*/) { throw std::logic_error("Unexpected invocation: TranslationVisitor::visit(const Observer&)"); }
};

bool is_definitely_disabled(const Guard& to_enable, const Guard& premise) {
	TranslationVisitor visitor;

	// translate expressions
	to_enable.accept(visitor);
	std::string to_enable_enc = visitor.result;
	premise.accept(visitor);
	std::string premise_enc = visitor.result;

	// create task: 1. variable declarations
	std::string task = "";
	for (auto name : visitor.names) {
		task = task + "(declare-const " + name + " Int) ";
	}

	// create task: 2. SAT challenge
	task = task + "(assert (and " + to_enable_enc + " " + premise_enc + ")) (check sat)";
	throw std::logic_error("Check SMTlib encoding: " + task);

	// solve
	z3::context context;
	auto parse_result = context.parse_string(task.c_str());
	z3::solver solver(context);
	auto result = solver.check();

	switch (result) {
		case z3::sat: return false;
		case z3::unsat: return true;
		case z3::unknown: return false;
	}
}

std::vector<const State*> abstract_post(const Observer& observer, const Transition& match, const State& state) {
	std::vector<const State*> result;
	for (const auto& transition : observer.transitions) {
		if (&transition->src == &state && &transition->label == &match.label && !is_definitely_disabled(*transition->guard, *match.guard)) {
			result.push_back(&transition->dst);
		}
	}
	result.push_back(&state); // TODO: implement a check and add 'state' conditionally // TODO: needed? the simulation relation should not be affected!?
	return result;
}

bool simulation_holds(const Observer& observer, SimulationEngine::SimulationRelation current, const State& to_simulate, const State& simulator) {
	for (const auto& transition : observer.transitions) {
		if (&transition->src == &to_simulate) {
			auto next_to_simulate = &transition->dst;
			auto simulator_post = abstract_post(observer, *transition, simulator);
			for (const auto& next_simulator : simulator_post) {
				auto required = std::make_pair(next_to_simulate, next_simulator);
				if (!current.count(required)) {
					return false;
				}
			}
		}
	}
	return true;
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
}

bool SimulationEngine::is_in_simulation_relation(const cola::State& state, const cola::State& other) {
	return all_simulations.count({ &state, &other }) > 0;
}
