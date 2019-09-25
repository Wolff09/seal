#pragma once
#ifndef PRTYPES_CHECK
#define PRTYPES_CHECK

#include <optional>
#include "cola/ast.hpp"
#include "types/observer.hpp"


namespace prtypes {

	bool type_check(const cola::Program& program, const SmrObserverStore& observer_store);

} // namespace prtypes

#endif