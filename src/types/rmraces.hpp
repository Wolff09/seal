#pragma once
#ifndef PRTYPES_RMRACES
#define PRTYPES_RMRACES

#include "cola/ast.hpp"
#include "types/error.hpp"
#include "types/observer.hpp"


namespace prtypes {

	void try_fix_pointer_race(cola::Program& program, const SmrObserverStore& observer_store, const UnsafeAssumeError& error, bool avoid_reoffending=true);
	
	void try_fix_pointer_race(cola::Program& program, const SmrObserverStore& observer_store, const UnsafeCallError& error);

	void try_fix_pointer_race(cola::Program& program, const SmrObserverStore& observer_store, const UnsafeDereferenceError& error);

} // namespace prtypes

#endif