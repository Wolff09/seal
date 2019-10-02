#pragma once
#ifndef PRTYPES_UTIL
#define PRTYPES_UTIL

#include <algorithm>
#include "types/types.hpp"


namespace prtypes {

	inline bool implies(bool lhs, bool rhs) {
		return !lhs || rhs;
	}

	template<typename Container, typename Key>
	inline bool has_binding(const Container& container, const Key& key) {
		return container.count(key) > 0;
	}

} // namespace prtypes

#endif