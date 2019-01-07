#pragma once
#ifndef PRTYPES_INFERENCE
#define PRTYPES_INFERENCE

#include <memory>
#include <string>
#include <set>
#include <map>
#include "types/check.hpp"
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "vata2/nfa.hh"

namespace prtypes {

	// TODO: we assume that observers have exactly 1 thread and 1 pointer observer variable

	using VataNfa = Vata2::Nfa::Nfa;
	using VataAlphabet = Vata2::Nfa::EnumAlphabet;
	using VataSymbol = std::string;

	enum struct SymbolicValue { THREAD, POINTER, OTHER };

	struct Symbol {
		const cola::Function& func;
		cola::Transition::Kind kind;
		std::vector<SymbolicValue> args;
		VataSymbol vata_symbol = "";
		std::uintptr_t vata_id = 0;
	};

	using Alphabet = std::vector<Symbol>;

	class Translator {
		private:
			const cola::Program& program;
			const GuaranteeSet all_guarantees;
			Alphabet alphabet;
			std::unique_ptr<VataAlphabet> vata_alphabet;
			std::map<const Guarantee*, VataNfa> guarantee2nfa;
			VataNfa universalnfa;

		public:
			Translator(const cola::Program& program, GuaranteeSet all_guarantees);

			const Alphabet& get_alphabet() const { return alphabet; }
			const VataAlphabet& get_vata_alphabet() const { return *vata_alphabet; }
			const GuaranteeSet& get_all_guarantees() const { return all_guarantees; }
			const VataNfa& get_universal_nfa() const { return universalnfa; }
			const VataNfa& nfa_for(const Guarantee& guarantee) const { return guarantee2nfa.at(&guarantee); }
			
			VataNfa to_nfa(const cola::Observer& observer);
	};

	class InferenceEngine final {
		public:
			using key_type = std::vector<bool>;
			using epsilon_inference_map_type = std::map<key_type, GuaranteeSet>;
			using command_inference_map_type = std::map<key_type, std::map<VataSymbol, GuaranteeSet>>;

		private:
			Translator translator;
			epsilon_inference_map_type inference_map_epsilon;
			command_inference_map_type inference_map_command;
			std::map<const Guarantee*, std::size_t> key_helper;
			std::size_t guarantee_count;

			std::size_t get_index(const Guarantee& guarantee);
			key_type get_key(const GuaranteeSet& guarantees);
			GuaranteeSet infer_command(const GuaranteeSet& guarantees, const cola::Command& command, const cola::VariableDeclaration* ptr);

		public:
			InferenceEngine(const cola::Program& program, const GuaranteeSet& all_guarantees);
			GuaranteeSet infer(const GuaranteeSet& guarantees);
			GuaranteeSet infer_enter(const GuaranteeSet& guarantees, const cola::Enter& enter, const cola::VariableDeclaration& ptr);
			GuaranteeSet infer_exit(const GuaranteeSet& guarantees, const cola::Exit& exit);
	};

} // namespace prtypes

#endif