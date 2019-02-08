#pragma once
#ifndef PRTYPES_UTIL
#define PRTYPES_UTIL


namespace prtypes {

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

	template<typename Container, typename Key>
	inline bool has_binding(const Container& container, const Key& key) {
		return container.count(key) > 0;
	}

} // namespace prtypes

#endif