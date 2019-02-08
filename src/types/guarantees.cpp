#include "types/guarantees.hpp"

using namespace cola;
using namespace prtypes;


GuaranteeTable::GuaranteeTable(const Observer& base, const Observer& impl) : smr_base_observer(std::move(base)), smr_impl_observer(std::move(impl)) {
	// TODO: create local (L) and active (A) guarantees
	throw std::logic_error("not yet implemented: GuaranteeTable::GuaranteeTable");
}

const Guarantee& GuaranteeTable::active_guarantee() const {
	assert(all_guarantees.size() > 0);
	return *all_guarantees.at(0).get();
}

const Guarantee& GuaranteeTable::local_guarantee() const {
	assert(all_guarantees.size() > 1);
	return *all_guarantees.at(1).get();
}

const Guarantee& GuaranteeTable::add_guarantee(std::unique_ptr<Observer> /*observer*/) {
	// TODO: create guarantee, compute if it is transient, compute if it is stable (and thus entails validity?)
	throw std::logic_error("not yet implemented: GuaranteeTable::add_guarantee(std::unique_ptr<Observer>)");
}
