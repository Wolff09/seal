#pragma once
#ifndef PRTYPES_CHECK
#define PRTYPES_CHECK

#include "cola/ast.hpp"
#include "types/guarantees.hpp"


namespace prtypes {

	bool type_check(const cola::Program& program, const GuaranteeTable& guarantee_table);

	void test();

} // namespace prtypes

#endif