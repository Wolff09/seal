#pragma once
#ifndef COLA_UTIL
#define COLA_UTIL


#include <memory>
#include <vector>
#include <map>
#include <string>
#include <ostream>
#include <optional>
#include "cola/ast.hpp"
#include "cola/observer.hpp"


namespace cola {

	std::unique_ptr<Expression> copy(const Expression& expr);

	std::unique_ptr<Invariant> copy(const Invariant& expr);

	std::unique_ptr<Command> copy(const Command& cmd);
	
	std::unique_ptr<Observer> copy(const Observer& observer);
	
	std::unique_ptr<ObserverVariable> copy(const ObserverVariable& variable);
	
	std::unique_ptr<Expression> negate(const Expression& expr);


	struct Indent {
		static const std::size_t indention_size = 4;
		std::size_t current_indent = 0;
		std::ostream& stream;
		Indent(std::ostream& stream) : stream(stream) {}
		Indent& operator++() { current_indent++; return *this; }
		Indent& operator--() { current_indent--; return *this; }
		Indent operator++(int) { Indent result(*this); ++(*this); return result; }
		Indent operator--(int) { Indent result(*this); --(*this); return result; }
		void print(std::ostream& stream) const { stream << std::string(current_indent*indention_size, ' '); }
		void operator()() const { print(stream); }
	};

	inline std::ostream& operator<<(std::ostream& stream, const Indent indent) { indent.print(stream); return stream; }

	void print(const Program& program, std::ostream& stream);
	
	void print(const Expression& expression, std::ostream& stream);
	
	void print(const Command& command, std::ostream& stream);
	
	void print(const Statement& statement, std::ostream& stream);

}

#endif