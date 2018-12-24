#pragma once

#include <istream>
#include <string>
#include <memory>
#include "ast/ast.hpp"


std::shared_ptr<Program> parse(std::string filename);

std::shared_ptr<Program> parse(std::istream& input);