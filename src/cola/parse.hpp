#pragma once
#ifndef COLA_PARSE
#define COLA_PARSE

#include <istream>
#include <string>
#include <memory>
#include "cola/ast.hpp"
#include "cola/observer.hpp"


namespace cola {

	std::shared_ptr<Program> parse_program(std::string filename);

	std::shared_ptr<Program> parse_program(std::istream& input);

	std::vector<std::unique_ptr<Observer>> parse_observer(std::string filename, const Program& program);

	std::vector<std::unique_ptr<Observer>> parse_observer(std::istream& input, const Program& program);

} // namespace cola

#endif