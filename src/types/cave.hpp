#pragma once
#ifndef PRTYPES_CAVE
#define PRTYPES_CAVE

#include "cola/ast.hpp"
#include "types/guarantees.hpp"
#include <ostream>


namespace prtypes {

	void to_cave_input(const cola::Program& program, const cola::Function& retire_function, const Guarantee& active, std::ostream& stream);

	std::string to_cave_input(const cola::Program& program, const cola::Function& retire_function, const Guarantee& active);

	bool discharge_assertions(const cola::Program& program, const cola::Function& retire_function, const Guarantee& active);

} // namespace prtypes

#endif