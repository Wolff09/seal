#pragma once
#ifndef PRTYPES_CAVE
#define PRTYPES_CAVE

#include "cola/ast.hpp"
#include "types/guarantees.hpp"
#include <ostream>
#include <string>
#include <vector>


namespace prtypes {

	bool discharge_assertions(const cola::Program& program, const cola::Function& retire_function, const Guarantee& active);

	bool discharge_assertions(const cola::Program& program, const cola::Function& retire_function, const Guarantee& active, const std::vector<std::reference_wrapper<const cola::Assert>>& whitelist);
	
	inline bool discharge_assertions(const cola::Program& program, const GuaranteeTable& guarantee_table) {
		return discharge_assertions(program, guarantee_table.observer_store.retire_function, guarantee_table.active_guarantee());
	}

	inline bool discharge_assertions(const cola::Program& program, const GuaranteeTable& guarantee_table, const std::vector<std::reference_wrapper<const cola::Assert>>& whitelist) {
		return discharge_assertions(program, guarantee_table.observer_store.retire_function, guarantee_table.active_guarantee(), std::move(whitelist));
	}

} // namespace prtypes

#endif