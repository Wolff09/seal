#pragma once
#ifndef PRTYPES_RMRACES
#define PRTYPES_RMRACES

#include "cola/ast.hpp"
#include "types/error.hpp"
#include "types/guarantees.hpp"


namespace prtypes {

	void try_fix_pointer_race(cola::Program& program, const GuaranteeTable&, const UnsafeAssumeError& error, bool avoid_reoffending=true);
	
	void try_fix_pointer_race(cola::Program& program, const GuaranteeTable& guarantee_table, const UnsafeCallError& error);

	void try_fix_pointer_race(cola::Program& program, const GuaranteeTable& guarantee_table, const UnsafeDereferenceError& error);

} // namespace prtypes

#endif