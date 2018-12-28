#pragma once
#ifndef COLA_PARSE
#define COLA_PARSE

#include <istream>
#include <string>
#include <memory>
#include "cola/ast.hpp"


namespace cola {

	std::shared_ptr<Program> parse(std::string filename);

	std::shared_ptr<Program> parse(std::istream& input);

} // namespace cola

#endif