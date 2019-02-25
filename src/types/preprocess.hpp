#pragma once
#ifndef PRTYPES_PREPROCESS
#define PRTYPES_PREPROCESS

#include "cola/ast.hpp"
#include "types/error.hpp"


namespace prtypes {

	void preprocess(cola::Program& program);

} // namespace prtypes

#endif