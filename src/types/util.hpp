#pragma once
#ifndef PRTYPES_UTIL
#define PRTYPES_UTIL

#include <algorithm>
#include "types/guarantees.hpp"


namespace prtypes {

	inline bool implies(bool lhs, bool rhs) {
		return !lhs || rhs;
	}

	inline bool entails_valid(const GuaranteeSet& guarantees) {
		for (const auto& guarantee : guarantees) {
			if (guarantee.get().entails_validity) {
				return true;
			}
		}
		return false;
	}

	inline GuaranteeSet prune_transient_guarantees(GuaranteeSet guarantees) {
		for (auto it = guarantees.cbegin(); it != guarantees.cend(); ) {
			if (it->get().is_transient) {
				it = guarantees.erase(it); // progresses it to the next element
			} else {
				++it;
			}
		}
		return guarantees;
	}

	inline GuaranteeSet merge(const GuaranteeSet& lhs, const GuaranteeSet& rhs) {
		GuaranteeSet result(lhs);
		result.insert(rhs.cbegin(), rhs.cend());
		return result;
	}

	inline GuaranteeSet intersection(const GuaranteeSet& lhs, const GuaranteeSet& rhs) {
		GuaranteeSet result;
		std::set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::inserter(result, result.begin()), GuaranteeSetComparator());
		return result;
	}

	inline TypeEnv intersection(const TypeEnv& lhs, const TypeEnv& rhs) {
		TypeEnv result;
		for (const auto& [decl, guarantees] : lhs) {
			auto rhs_find = rhs.find(decl);
			if (rhs_find != rhs.end()) {
				result.insert({ decl, intersection(guarantees, rhs_find->second) });
			}
		}
		return result;
	}

	template<typename Container, typename Key>
	inline bool has_binding(const Container& container, const Key& key) {
		return container.count(key) > 0;
	}

} // namespace prtypes

#endif