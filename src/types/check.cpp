#include "types/check.hpp"
#include "types/checker.hpp"

using namespace cola;


bool prtypes::type_check(const cola::Program& program, const SmrObserverStore& observer_store) {
	TypeContext context(observer_store);
	TypeChecker checker(program, context);
	return checker.is_well_typed();
}
