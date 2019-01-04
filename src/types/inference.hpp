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

	enum struct SymbolicValue { THREAD, POINTER, OTHER };

	struct Symbol {
		const cola::Function& func;
		std::vector<SymbolicValue> args;
		std::uintptr_t vata_id = 0; // TODO: how to do this properly?
	};

	using Alphabet = std::vector<Symbol>;

	using VataNfa = Vata2::Nfa::Nfa;
	using VataAlphabet = Vata2::Nfa::EnumAlphabet;
	using VataSymbol = std::string;

	class Translator {
		private:
			const cola::Program& program;
			Alphabet alphabet;
			std::unique_ptr<VataAlphabet> vata_alphabet;
			// TODO?: map<uintptr_t, Symbol> vata2symbol;
			// TODO?: enter/exit to function map

		public:
			Translator(const cola::Program& program);

			const Alphabet& get_alphabet() const { return alphabet; }
			const VataAlphabet& get_vata_alphabet() const { return *vata_alphabet; }

			VataNfa to_nfa(const cola::Observer& observer);
			Symbol to_symbol(const cola::Command& command, const cola::VariableDeclaration* ptr=nullptr);
			VataSymbol to_vata(const cola::Command& command, const cola::VariableDeclaration* ptr=nullptr);
			VataSymbol to_vata(Symbol symbol);
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
			std::map<std::reference_wrapper<const Guarantee>, std::size_t> key_helper;
			std::size_t guarantee_count;

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