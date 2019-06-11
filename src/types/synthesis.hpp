#pragma once
#ifndef PRTYPES_SYNTHESIS
#define PRTYPES_SYNTHESIS

#include <vector>
#include <memory>
#include "cola/observer.hpp"
#include "types/guarantees.hpp"


namespace prtypes {

	/**
	 * Synthesizes guarantees for the SMR observers  represented by <code>guarantee_table.observer_store</code>.
	 * This function does not consider the actual cross product of <code>guarantee_table.observer_store.impl_observer</code>.
	 * Instead, it considers individually each of those observers, that is, its cross product with <code>guarantee_table.observer_store.base_observer</code>.
	 */
	void populate_guarantee_table_with_synthesized_guarantees(GuaranteeTable& guarantee_table);

} // namespace prtypes

#endif