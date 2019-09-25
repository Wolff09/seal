#include "cola/parser/ObserverBuilder.hpp"

#include "cola/util.hpp"
#include <sstream>

using namespace cola;



std::vector<std::unique_ptr<Observer>> ObserverBuilder::buildFrom(CoLaParser::ObserverContext* parseTree, const Program& program) {
	ObserverBuilder builder(program);
	parseTree->accept(&builder);
	return std::move(builder._observer_list);
}


inline const Function& free_function() {
	static const Function& free_function = Observer::free_function();
	return free_function;
}

inline const Type& get_ptr_type() {
	static const Type& ptrtype = free_function().args.at(0)->type;
	return ptrtype;
}

void ObserverBuilder::check_name_clash(std::string name) {
	// check for maps and this.
	if (name == "this") {
		throw std::logic_error("Parsing error: 'this' is a reserved key word.");
	}
	if (_name2state.count(name) > 0) {
		throw std::logic_error("Parsing error: '" + name + "' is already defined as a state.");
	}
	if (_name2threadvar.count(name) > 0) {
		throw std::logic_error("Parsing error: '" + name + "' is already defined as a thread variable.");
	}
	if (_name2ptrvar.count(name) > 0) {
		throw std::logic_error("Parsing error: '" + name + "' is already defined as a pointer variable.");
	}
}

template<typename T>
inline T find(const std::unordered_map<std::string, T>& container, std::string name, std::string error) {
	auto find = container.find(name);
	if (find == container.end()) {
		throw std::logic_error("Parsing error: " + error + " '" + name + "'.");
	}
	return find->second;
}

State& ObserverBuilder::find_state(std::string name) {
	return find(_name2state, name, "undefined state");
}

const Function& ObserverBuilder::find_function(std::string name) {
	if (name == free_function().name) {
		return free_function();
	} else {
		return *find(_name2function, name, "unknown function");
	}
}

const ProgramObserverVariable& ObserverBuilder::find_ptrvar(std::string name) {
	return find(_name2ptrvar, name, "undefined pointer variable");
}

const ThreadObserverVariable& ObserverBuilder::find_threadvar(std::string name) {
	return find(_name2threadvar, name, "undefined thread variable");
}


ObserverBuilder::ObserverBuilder(const Program& program) {
	for (const auto& function : program.functions) {
		if (function->kind == Function::SMR) {
			_name2function[function->name] = function.get();
		}
	}
}


antlrcpp::Any ObserverBuilder::visitObserverList(CoLaParser::ObserverListContext* context) {
	for (const auto& observerContext : context->obs_def()) {
		observerContext->accept(this);
	}
	return nullptr;
}

antlrcpp::Any ObserverBuilder::visitObserverDefinition(CoLaParser::ObserverDefinitionContext* context) {
	// TODO: use observer name

	// reset from previous runs
	_observer = nullptr;
	_current_arg = nullptr;
	_name2state.clear();
	_name2threadvar.clear();
	_name2ptrvar.clear();

	// parse observer
	auto new_observer = std::make_unique<Observer>(context->name->getText());
	_observer = new_observer.get();

	if (context->positive && !context->negative) {
		_observer->negative_specification = false;
	} else if (!context->positive && context->negative) {
		_observer->negative_specification = true;
	} else {
		throw std::logic_error("Parsing error: observer is positive and negative.");
	}
	context->var_list()->accept(this);
	context->state_list()->accept(this);
	context->trans_list()->accept(this);
	_observer_list.push_back(std::move(new_observer));

	return nullptr;
}

antlrcpp::Any ObserverBuilder::visitObserverVariableList(CoLaParser::ObserverVariableListContext* context) {
	for (const auto& variableContext : context->obs_var()) {
		variableContext->accept(this);
	}
	return nullptr;
}

antlrcpp::Any ObserverBuilder::visitObserverVariable(CoLaParser::ObserverVariableContext* context) {
	std::string name = context->name->getText();
	check_name_clash(name);
	if (context->pointer && !context->thread) {
		auto var = std::make_unique<ProgramObserverVariable>(std::make_unique<VariableDeclaration>(name, get_ptr_type(), false));
		_name2ptrvar.insert({ name, *var.get() });
		_observer->variables.push_back(std::move(var));
	} else if (!context->pointer && context->thread) {
		auto var = std::make_unique<ThreadObserverVariable>(name);
		_name2threadvar.insert({ name, *var.get() });
		_observer->variables.push_back(std::move(var));
	} else {
		throw std::logic_error("Parsing error: variable declaration malformed.");
	}
	return nullptr;
}

antlrcpp::Any ObserverBuilder::visitObserverStateList(CoLaParser::ObserverStateListContext* context) {
	for (const auto& stateContext : context->state()) {
		stateContext->accept(this);
	}
	return nullptr;
}

antlrcpp::Any ObserverBuilder::visitObserverState(CoLaParser::ObserverStateContext* context) {
	auto state = std::make_unique<State>();
	if (context->verbose) {
		state->name = context->verbose->getText();
	}
	if (context->initial) {
		state->initial = true;
	}
	if (context->final) {
		state->final = true;
	}
	std::string id = context->name->getText();
	check_name_clash(id);
	_name2state.insert({ id, *state.get() });
	_observer->states.push_back(std::move(state));
	return nullptr;
}

antlrcpp::Any ObserverBuilder::visitObserverTransitionList(CoLaParser::ObserverTransitionListContext* context) {
	for (const auto& transitionContext : context->transition()) {
		transitionContext->accept(this);
	}
	return nullptr;
}

struct TrueGuardChecker final : public ObserverVisitor {
	bool result = false;
	void visit(const ThreadObserverVariable& /*obj*/) override { /* do nothing */ }
	void visit(const ProgramObserverVariable& /*obj*/) override { /* do nothing */ }
	void visit(const SelfGuardVariable& /*obj*/) override { /* do nothing */ }
	void visit(const ArgumentGuardVariable& /*obj*/) override { /* do nothing */ }
	void visit(const TrueGuard& /*obj*/) override { result = true; }
	void visit(const ConjunctionGuard& /*obj*/) override { /* do nothing */ }
	void visit(const EqGuard& /*obj*/) override { /* do nothing */ }
	void visit(const NeqGuard& /*obj*/) override { /* do nothing */ }
	void visit(const State& /*obj*/) override { /* do nothing */ }
	void visit(const Transition& /*obj*/) override { /* do nothing */ }
	void visit(const Observer& /*obj*/) override { /* do nothing */ }
};

inline bool is_true_guard(const Guard& guard) {
	TrueGuardChecker visitor;
	guard.accept(visitor);
	return visitor.result;
}

template<typename C, typename T>
void add_raw_guard(C& container, T* ptr) {
	std::unique_ptr<Guard> result;
	result.reset(ptr);
	if (!is_true_guard(*result)) {
		container.push_back(std::move(result));
	}
}

antlrcpp::Any ObserverBuilder::visitObserverTransition(CoLaParser::ObserverTransitionContext* context) {
	auto& src = find_state(context->src->getText());
	const auto& dst = find_state(context->dst->getText());
	const auto& label = find_function(context->name->getText());

	// TODO: prevent free from having enter/exit
	// TODO: prevent free from having implicit this?
	Transition::Kind kind = Transition::INVOCATION;
	if (context->enter && !context->exit) {
		kind = Transition::INVOCATION;
		if (context->argsguard.size() != label.args.size()) {
			throw std::logic_error("Parsing error: function '" + label.name + "' has " + std::to_string(label.args.size()) + " parameters but " + std::to_string(context->argsguard.size()) + " were provided.");
		}

	} else if (!context->enter && context->exit) {
		kind = Transition::RESPONSE;
		if (context->argsguard.size() != 0) {
			throw std::logic_error("Parsing error: 'exit' definition for function '" + label.name + "' specifies paramters.");
		}

	} else if (!context->enter && !context->exit) {
		if (&label != &free_function()) {
			throw std::logic_error("Parsing error: missing enter/exit in transition.");
		}
	} else {
		throw std::logic_error("Parsing error: malformed transition.");
	}

	// create guard
	_current_arg = nullptr; // sets parsing of first argument to implicit this
	std::vector<std::unique_ptr<Guard>> conjuncts;
	add_raw_guard(conjuncts, context->thisguard->accept(this).as<Guard*>());
	auto iterator = label.args.cbegin();
	for (const auto& guardContext : context->argsguard) {
		_current_arg = iterator->get();
		add_raw_guard(conjuncts, guardContext->accept(this).as<Guard*>());
		iterator++;
	}
	if (conjuncts.size() == 0) {
		conjuncts.push_back(std::make_unique<TrueGuard>());
	}

	src.transitions.push_back(std::make_unique<Transition>(src, dst, label, kind, std::make_unique<ConjunctionGuard>(std::move(conjuncts))));
	return nullptr;
}

antlrcpp::Any ObserverBuilder::visitObserverGuardTrue(CoLaParser::ObserverGuardTrueContext* /*context*/) {
	return (Guard*) new TrueGuard();
}

antlrcpp::Any ObserverBuilder::visitObserverGuardIdentifierEq(CoLaParser::ObserverGuardIdentifierEqContext* context) {
	if (_current_arg) {
		// pointer case
		return (Guard*) new EqGuard(find_ptrvar(context->name->getText()), std::make_unique<ArgumentGuardVariable>(*_current_arg));

	} else {
		// thread (this) case
		return (Guard*) new EqGuard(find_threadvar(context->name->getText()), std::make_unique<SelfGuardVariable>());
	}
}

antlrcpp::Any ObserverBuilder::visitObserverGuardIdentifierNeq(CoLaParser::ObserverGuardIdentifierNeqContext* context) {
	if (_current_arg) {
		// pointer case
		return (Guard*) new NeqGuard(find_ptrvar(context->name->getText()), std::make_unique<ArgumentGuardVariable>(*_current_arg));

	} else {
		// thread (this) case
		return (Guard*) new NeqGuard(find_threadvar(context->name->getText()), std::make_unique<SelfGuardVariable>());
	}
}


