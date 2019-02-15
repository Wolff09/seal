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


namespace cola {

	std::unique_ptr<Expression> copy(const Expression& expr);

	void print(const Program& program, std::ostream& stream);

}

#endif