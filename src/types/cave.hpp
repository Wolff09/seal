#pragma once
#ifndef PRTYPES_CAVE
#define PRTYPES_CAVE

#include "cola/ast.hpp"
#include "types/guarantees.hpp"
#include <ostream>
#include <string>
#include <vector>


namespace prtypes {

	bool check_linearizability(const cola::Program& program);

	bool discharge_assertions(const cola::Program& program, const cola::Function& retire_function);

	bool discharge_assertions(const cola::Program& program, const cola::Function& retire_function, const std::vector<std::reference_wrapper<const cola::Assert>>& whitelist);
	
	inline bool discharge_assertions(const cola::Program& program, const SmrObserverStore& observer_store) {
		return discharge_assertions(program, observer_store.retire_function);
	}
	
	inline bool discharge_assertions(const cola::Program& program, const GuaranteeTable& guarantee_table) {
		return discharge_assertions(program, guarantee_table.observer_store);
	}

	inline bool discharge_assertions(const cola::Program& program, const SmrObserverStore& observer_store, const std::vector<std::reference_wrapper<const cola::Assert>>& whitelist) {
		return discharge_assertions(program, observer_store.retire_function, std::move(whitelist));
	}

	inline bool discharge_assertions(const cola::Program& program, const GuaranteeTable& guarantee_table, const std::vector<std::reference_wrapper<const cola::Assert>>& whitelist) {
		return discharge_assertions(program, guarantee_table.observer_store, std::move(whitelist));
	}

} // namespace prtypes

#endif