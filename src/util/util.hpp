#pragma once
#ifndef COLA_UTIL
#define COLA_UTIL


#include <memory>
#include <vector>
#include <map>
#include <string>
#include "ast/ast.hpp"


namespace cola {

	std::unique_ptr<Expression> copy(const Expression& expr);

	void pretty_print(/*TODO: iostream*/);
}

#endif