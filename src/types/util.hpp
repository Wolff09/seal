#pragma once
#ifndef PRTYPES_UTIL
#define PRTYPES_UTIL

// #include <memory>
// #include <vector>
// #include <string>
#include "cola/observer.hpp"


namespace prtypes {

	void observer_to_symbolic_nfa(const cola::Observer& observer);


} // namespace prtypes

#endif