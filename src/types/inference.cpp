#include "inference.hpp"
#include "vata2/nfa.hh"
#include "cola/ast.hpp"
#include <vector>
#include <sstream>
#include <memory>

using namespace cola;
using namespace prtypes;


//===================== helper

template<typename T>
void copy_vector_contents(std::vector<T>& to, std::vector<T>& from) {
	for (T elem : from) {
		to.push_back(elem);
	}
}

bool are_symbols_equal(const Symbol& symbol, const Symbol& other) {
	// returns true iff symbol and other aggree on func and contents or args members
	if (&symbol.func != &other.func) {
		return false;
	}
	if (symbol.kind != other.kind) {
		return false;
	}
	assert(symbol.args.size() == other.args.size());
	for (std::size_t i = 0; i < symbol.args.size(); i++) {
		if (symbol.args.at(i) != other.args.at(i)) {
			return false;
		}
	}
	return true;
}

std::string symbolic_value_to_string(SymbolicValue value) {
	switch(value) {
		case SymbolicValue::THREAD: return "T";
		case SymbolicValue::POINTER: return "P";
		case SymbolicValue::OTHER: return "$";
	}
}

std::string symbol_to_string(const Symbol& symbol) {
	// return std::to_string(symbol.vata_id);
	std::stringstream builder;
	builder << symbol.func.name;
	builder << "(";
	if (!symbol.args.empty()) {
		auto it = symbol.args.begin();
		builder << symbolic_value_to_string(*it);
		it++;
		for (; it != symbol.args.end(); it++) {
			builder << ",";
			builder << symbolic_value_to_string(*it);
		}
	}
	builder << ")";
	return builder.str();
}


//===================== generating alphabet

using SymbolicArgumentMap = std::map<const VariableDeclaration*, std::set<SymbolicValue>>;
using SymbolicArgumentList = std::vector<std::set<SymbolicValue>>;

struct ValuationPruningVisitor final : ObserverVisitor {
	SymbolicArgumentMap& arg2val;
	const VariableDeclaration* rhs;
	SymbolicValue lhs;

	void remove(const VariableDeclaration* key, SymbolicValue value) {
		arg2val.at(key).erase(value);
	}

	ValuationPruningVisitor(SymbolicArgumentMap& arg2val) : arg2val(arg2val) {}

	void visit(const ThreadObserverVariable& /*obj*/) override {
		lhs = SymbolicValue::THREAD;
	}

	void visit(const ProgramObserverVariable& variable) override {
		if (variable.decl->type.sort != Sort::PTR) {
			throw std::logic_error("Unsupported observer variable sort; expected 'Sort::PTR' here.");
		}
		lhs = SymbolicValue::POINTER;
	}

	void visit(const SelfGuardVariable& /*obj*/) override {
		rhs = nullptr;
	}

	void visit(const ArgumentGuardVariable& variable) override {
		rhs = &variable.decl;
	}

	void visit(const EqGuard& guard) override {
		guard.lhs.accept(*this);
		guard.rhs.accept(*this);
		remove(rhs, SymbolicValue::OTHER);
	}

	void visit(const NeqGuard& guard) override {
		guard.lhs.accept(*this);
		guard.rhs.accept(*this);
		remove(rhs, lhs);
	}

	void visit(const ConjunctionGuard& guard) override {
		guard.lhs->accept(*this);
		guard.rhs->accept(*this);
	}

	void visit(const TrueGuard& /*obj*/) override { /* do nothing */ }

	void visit(const State& /*obj*/) override { throw std::logic_error("Unexpected invocation (ThreadValuationVisitor::visit(const State&))"); }
	void visit(const Transition& /*obj*/) override { throw std::logic_error("Unexpected invocation (ThreadValuationVisitor::visit(const Transition&))"); }
	void visit(const Observer& /*obj*/) override { throw std::logic_error("Unexpected invocation (ThreadValuationVisitor::visit(const Observer&))"); }
};

SymbolicArgumentList get_all_possible_arguments(const Function& function, Transition::Kind kind, const Guard* guard=nullptr) {
	SymbolicArgumentMap arg2val;
	
	// implicit thread argument (self)
	arg2val[nullptr] = { SymbolicValue::THREAD, SymbolicValue::OTHER };

	if (kind == Transition::INVOCATION) {
		// explicit arguments from function.args (only for INVOCATION, not for RESPONSE)
		for (const auto& arg : function.args) {
			if (arg->type.sort == Sort::PTR) {
				arg2val[arg.get()] = { SymbolicValue::POINTER, SymbolicValue::OTHER };
			} else {
				arg2val[arg.get()] = { SymbolicValue::OTHER };
			}
		}

	} else {
		assert(kind == Transition::RESPONSE);
	}

	// prune that map, provided guard
	if (guard) {
		ValuationPruningVisitor visitor(arg2val);
		guard->accept(visitor);
	}

	// translate map into list
	SymbolicArgumentList result;
	result.push_back(arg2val.at(nullptr));
	if (kind == Transition::INVOCATION) {
		// again, arguments only for invocations
		for (const auto& arg : function.args) {
			result.push_back(arg2val.at(arg.get()));
		}
	} else {
		assert(kind == Transition::RESPONSE);
	}

	// done
	return result;
}

bool is_enabled(const SymbolicArgumentList& arglist) {
	// not enalbed if some entry has no symbolic values
	for (const auto& vals : arglist) {
		if (vals.empty()) {
			return false;
		}
	}
	return true;
}

std::vector<Symbol> make_symbols_for_function(const Function& function, Transition::Kind kind, const Guard* guard=nullptr) {
	std::vector<Symbol> result;
	SymbolicArgumentList arglist = get_all_possible_arguments(function, kind, guard);

	// return empty list if guard is present and not enabled
	if (!is_enabled(arglist)) {
		return result;
	}

	// add initial to-be-extended symbol
	result.push_back({ function, kind, {} });

	// flatten the list of sets of values into a list of values
	for (const auto& set : arglist) {
		std::vector<Symbol> new_result;
		for (SymbolicValue val : set) {
			std::vector<Symbol> copy(result);
			for (auto symbol : copy) {
				symbol.args.push_back(val);
				// new_result.push_back(symbol);
			}
			copy_vector_contents(new_result, copy);
		}

		result = std::move(new_result);
	}

	// done
	return result;
}

Alphabet alphabet_from_program(const Program& program) {
	Alphabet result;

	// add SMR functions defined by program
	for (const auto& function : program.functions) {
		for (auto kind : { Transition::INVOCATION, Transition::RESPONSE }) {
			auto function_symbols = make_symbols_for_function(*function, kind);
			copy_vector_contents(result, function_symbols);
		}
	}

	// add special free function (only invocation)
	auto free_symbols = make_symbols_for_function(Observer::free_function(), Transition::INVOCATION);
	copy_vector_contents(result, free_symbols);

	// set ids
	std::uintptr_t counter = 0;
	for (auto& symbol : result) {
		symbol.vata_id = counter++;
	}

	return result;
}


//===================== generating NFA

Symbol make_symbol_with_vata_id(Symbol symbol, const Alphabet& alphabet) {
	for (const auto& alphabet_symbol : alphabet) {
		if (are_symbols_equal(symbol, alphabet_symbol)) {
			return alphabet_symbol;
		}
	}
	throw std::logic_error("Unexpected symbol occured, not found in alphabet; cannot recover.");
}

std::vector<Symbol> get_symbol_for_transition(const Transition& transition, const Alphabet& alphabet) {
	std::vector<Symbol> result;

	// get all possible symbols allowed by the function, pruned by transition guard
	auto symbols = make_symbols_for_function(transition.label, transition.kind, transition.guard.get());

	// resulting symbols lack vata_id -> look up in alphabet
	for (auto symbol : symbols) {
		result.push_back(make_symbol_with_vata_id(symbol, alphabet));
	}

	return result;
}

VataNfa translate_observer(const Observer& observer, const Alphabet& alphabet, const VataAlphabet& vata_alphabet) {
	VataNfa nfa;

	// assign vata compatible ids to states of the observer
	// vata nfa has no explicit notion of states (except initial/final), implicitly added by transitions
	std::map<const State*, std::uintptr_t> state2vata;
	std::uintptr_t counter = 0;
	for (const auto& state : observer.states) {
		auto encoding = counter++;
		state2vata[state.get()] = encoding;
		if (state->initial) {
			nfa.add_initial(encoding);
		}
		if (state->final) {
			nfa.add_final(encoding);
		}
	}

	// copy transitions from observer to nfa; map transition label+guard to symbols from the alphabet
	std::map<const State*, std::set<std::uintptr_t>> outgoing_vata;
	for (const auto& transition : observer.transitions) {
		std::uintptr_t src_state = state2vata.at(&transition->src);
		std::uintptr_t dst_state = state2vata.at(&transition->dst);
		auto symbols = get_symbol_for_transition(*transition, alphabet);
		for (const auto& sym : symbols) {
			nfa.add_trans(src_state, sym.vata_id, dst_state);
			outgoing_vata[&transition->src].insert({ sym.vata_id });
		}
	}

	// make nfa complete by adding self-loops for "missing" transitions
	auto missing_symbols = [&](const State& state) -> std::set<std::uintptr_t> {
		std::set<std::uintptr_t> result;
		auto outgoing = outgoing_vata.at(&state);
		for (const auto& symbol : alphabet) {
			if (outgoing.count(symbol.vata_id) == 0) {
				result.insert(symbol.vata_id);
			}
		}
		return result;
	};
	for (const auto& state : observer.states) {
		auto missing = missing_symbols(*state);
		std::uintptr_t vataid = state2vata.at(state.get());
		for (auto symbol : missing) {
			nfa.add_trans(vataid, symbol, vataid);
		}
	}

	// complement nfa if negative specification
	if (observer.negative_specification) {
		return Vata2::Nfa::complement(nfa, vata_alphabet);
	} else {
		return nfa;
	}
}


//===================== translation

std::unique_ptr<VataAlphabet> convert_alphabet(Alphabet& alphabet) {
	// computes vata alphabet and updates the given alphabet with vata internal information
	
	// get vata encoding (string) for each symbol; also store that info in the symbol
	std::set<VataSymbol> keys;
	for (auto& symbol : alphabet) {
		VataSymbol vata_encoding = symbol_to_string(symbol);
		symbol.vata_symbol = vata_encoding;
		auto res = keys.insert(vata_encoding);
		if (!res.second) {
			throw std::logic_error("Alphabet conversion failed; vata encodings are not unique.");
		}
	}

	// create vata alphabet
	auto result = std::make_unique<VataAlphabet>(keys.begin(), keys.end());

	// extend alphabet with unique vata identifiers
	for (auto& symbol : alphabet) {
		symbol.vata_id = result->translate_symb(symbol.vata_symbol);
	}

	// done
	return result;
}

Translator::Translator(const Program& program_, GuaranteeSet all_guarantees_) : program(program_), all_guarantees(all_guarantees_) {
	alphabet = alphabet_from_program(program);
	vata_alphabet = convert_alphabet(alphabet);

	for (const auto& guarantee : all_guarantees) {
		auto nfa = to_nfa(*guarantee.get().observer);
		auto res = guarantee2nfa.insert({ guarantee, nfa });
		if (!res.second) {
			throw std::logic_error("Unexpected non-unique guarantees in set.");
		}
	}

	// TODO: init universalnfa;
	throw std::logic_error("not yet implemented (initialization of universalnfa)");
}

VataNfa Translator::to_nfa(const Observer& observer) {
	return translate_observer(observer, alphabet, *vata_alphabet);
}

// TODO: maybe rewrite the definition of the following functions depending on what we need
Symbol Translator::to_symbol(const Command& command, const VariableDeclaration* ptr) {
	// assumption: command is of type enter => ptr != nullptr; command is of type exit => ptr == nullptr; command is either of type enter or exit
	throw std::logic_error("not yet implemented (to_symbol)");
}

VataSymbol Translator::to_vata(Symbol symbol) {
	throw std::logic_error("not yet implemented (to_vata)");
}

VataSymbol Translator::to_vata(const cola::Command& command, const cola::VariableDeclaration* ptr) {
	return to_vata(to_symbol(command, ptr));
}

//===================== inference

VataNfa nfa_intersection_for_guarantees(Translator& translator, const GuaranteeSet& guarantees) {
	// returns universal automaton if guarantees are empty
	VataNfa result = translator.get_universal_nfa();
	for (auto iterator = guarantees.begin(); iterator != guarantees.end(); iterator++) {
		const VataNfa& nfa = translator.nfa_for(*iterator);
		VataNfa new_result = Vata2::Nfa::intersection(result, nfa);
		result = std::move(new_result);
	}
	return result;
}

bool nfa_inclusion(Translator& translator, const VataNfa& subset, const VataNfa& superset) {
	return is_incl (subset, superset, translator.get_vata_alphabet());
}

GuaranteeSet infer_guarantees(Translator& translator, const VataNfa& from_nfa) {
	GuaranteeSet result;
	for (const Guarantee& guarantee : translator.get_all_guarantees()) {
		auto infer = translator.nfa_for(guarantee);
		if (nfa_inclusion(translator, from_nfa, infer)) {
			result.insert(guarantee);
		}
	}
	return result;
}

std::vector<Symbol> compute_symbols_for_event(Translator& translator, const Command& command, const VariableDeclaration* ptr) {
	if (ptr) {
		// enter event
		auto& enter = static_cast<const Enter&>(command);
		// TODO: create handcrafted guard for proper restriction; this requires to match the expressions from the invocations to the argument from the function declaration
		throw std::logic_error("not yet implemented (compute_symbols_for_event)");
		// return make_symbols_for_function(enter.decl, Transition::INVOCATION, guard);

	} else {
		// exit event: the implicit thread argument is not OTHER by definition (see paper)
		auto& exit = static_cast<const Exit&>(command);
		Symbol result({ exit.decl, Transition::RESPONSE, { SymbolicValue::THREAD } });
		return { result };
	}
}

VataNfa nfa_concatenation_with_symbols(Translator& translator, VataNfa nfa, std::vector<Symbol> symbols) {
	// concat intersection with symbol (with all symbols at once? probably yes)
	// should the result be complete?? i.e. should symbols lead to final states and complement-symbols stay in their current state???
	throw std::logic_error("not yet implemented (nfa_concatenation_with_symbols)");
}

GuaranteeSet compute_inference_epsilon(Translator& translator, const GuaranteeSet& guarantees) {
	auto intersection = nfa_intersection_for_guarantees(translator, guarantees);
	return infer_guarantees(translator, intersection); // TODO: one does not need to consider guarantees already present
}

GuaranteeSet compute_inference_command(Translator& translator, const GuaranteeSet& guarantees, const Command& command, const VariableDeclaration* ptr) {
	// TODO: this function should take a (vector of) symbol instead of command and ptr
	auto intersection = nfa_intersection_for_guarantees(translator, guarantees);
	auto symbols = compute_symbols_for_event(translator, command, ptr);
	auto concatenation = nfa_concatenation_with_symbols(translator, std::move(intersection), std::move(symbols));
	return infer_guarantees(translator, concatenation);
}

InferenceEngine::key_type InferenceEngine::get_key(const GuaranteeSet& guarantees) {
	key_type result(guarantee_count, false);
	for (const auto& guarantee : guarantees) {
		result.at(key_helper.at(guarantee)) = true;
	}
	return result;
}

InferenceEngine::InferenceEngine(const Program& program, const GuaranteeSet& all_guarantees) : translator(program, all_guarantees) {
	// init key_helper
	guarantee_count = 0;
	for (const auto& guarantee : all_guarantees) {
		auto res = key_helper.insert({ guarantee, guarantee_count++ });
		if (!res.second) {
			throw std::logic_error("Failed to create key_helper.");
		}
	}
	assert(guarantee_count == all_guarantees.size());
	assert(guarantee_count == key_helper.size());
}

GuaranteeSet InferenceEngine::infer(const GuaranteeSet& guarantees) {
	auto key = get_key(guarantees);

	if (inference_map_epsilon.count(key) == 0) {
		inference_map_epsilon[key] = compute_inference_epsilon(translator, guarantees);
	}

	return inference_map_epsilon.at(key);
}

GuaranteeSet InferenceEngine::infer_command(const GuaranteeSet& guarantees, const Command& command, const VariableDeclaration* ptr) {
	auto key = get_key(guarantees);
	auto sym = translator.to_vata(command, ptr); // TODO: this gives a set, no??
	// TODO: question: just always compute inference, compute it for a set of symbols, or for individual symbols and do intersection on guarteeset

	auto map = inference_map_command[key];
	if (map.count(sym) == 0) {
		map[sym] = compute_inference_command(translator, guarantees, command, ptr);
	}

	return map.at(sym);
}

GuaranteeSet InferenceEngine::infer_enter(const GuaranteeSet& guarantees, const Enter& command, const VariableDeclaration& ptr) {
	return infer_command(guarantees, command, &ptr);
}

GuaranteeSet InferenceEngine::infer_exit(const GuaranteeSet& guarantees, const Exit& command) {
	return infer_command(guarantees, command, nullptr);
}
