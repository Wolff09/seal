#include "types/guarantees.hpp"
#include "types/factory.hpp"

using namespace cola;
using namespace prtypes;


std::unique_ptr<Guarantee> make_guarantee(const SmrObserverStore& store, std::string name, bool transient, bool valid) {
	auto result = std::make_unique<Guarantee>(name, prtypes::make_active_local_guarantee_observer(store.retire_function, name));
	result->is_transient = transient;
	result->entails_validity = valid;
	return result;
}

std::unique_ptr<Guarantee> make_active_guarantee(const SmrObserverStore& store) {
	return make_guarantee(store, "Active", true, true);
}

std::unique_ptr<Guarantee> make_local_guarantee(const SmrObserverStore& store) {
	return make_guarantee(store, "Local", false, true);
}

GuaranteeTable::GuaranteeTable(const SmrObserverStore& store) : observer_store(store) {
	// TODO: perform checks on active/local guarantees
	all_guarantees.push_back(make_active_guarantee(store));
	all_guarantees.push_back(make_local_guarantee(store));
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
	// can we use inference with Observer::free_fun here?
	throw std::logic_error("not yet implemented: GuaranteeTable::add_guarantee(std::unique_ptr<Observer>)");
}
