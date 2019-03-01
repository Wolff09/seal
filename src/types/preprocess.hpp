#pragma once
#ifndef PRTYPES_PREPROCESS
#define PRTYPES_PREPROCESS

#include "cola/ast.hpp"
#include "types/error.hpp"


namespace prtypes {

	void preprocess(cola::Program& program, const cola::Function& retire_function);

} // namespace prtypes

#endif