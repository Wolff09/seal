#pragma once
#ifndef PRTYPES_GUARANTEES
#define PRTYPES_GUARANTEES

#include <memory>
#include <string>
#include <set>
#include <map>
#include "cola/ast.hpp"
#include "cola/observer.hpp"


namespace prtypes {

	struct Guarantee final {
		std::string name;
		std::unique_ptr<cola::Observer> observer;
		bool is_transient = true; // false ==> stable under interference // TODO: initialize this in the constructor?
		bool entails_validity = false; // true ==> pointer is guaranteed to be valid
		Guarantee(std::string name_, std::unique_ptr<cola::Observer> observer_) : name(name_), observer(std::move(observer_)) {
			assert(observer);
		}
	};


	struct GuaranteeTableIterator final : public std::input_iterator_tag {
		using iterator_t = std::vector<std::unique_ptr<Guarantee>>::const_iterator;
		iterator_t iterator;
		GuaranteeTableIterator(iterator_t it) : iterator(it) {}
		GuaranteeTableIterator& operator++() { // Pre-increment
			iterator++;
			return *this;
		}
		GuaranteeTableIterator operator++(int) { // Post-increment
			GuaranteeTableIterator tmp(*this);
			iterator++;
			return tmp;
		}
		bool operator ==(const GuaranteeTableIterator& other) const { return iterator == other.iterator; }
		bool operator !=(const GuaranteeTableIterator& other) const { return iterator != other.iterator; }
		const Guarantee& operator*() const { return *(iterator->get()); }
		const Guarantee& operator->() const { return *(iterator->get()); }
	};

	struct GuaranteeTable final {
		std::vector<std::unique_ptr<Guarantee>> all_guarantees;
		const cola::Observer& smr_base_observer;
		const cola::Observer& smr_impl_observer;
		
		GuaranteeTable(const cola::Observer& base, const cola::Observer& impl);
		const Guarantee& active_guarantee() const;
		const Guarantee& local_guarantee() const;
		const Guarantee& add_guarantee(std::unique_ptr<cola::Observer> observer);

		GuaranteeTableIterator begin() const { return GuaranteeTableIterator(all_guarantees.cbegin()); }
		GuaranteeTableIterator end() const { return GuaranteeTableIterator(all_guarantees.cend()); }
	};


	struct GuaranteeSetComparator {
		bool operator() (const std::reference_wrapper<const Guarantee>& lhs, const std::reference_wrapper<const Guarantee>& rhs) const {
			return &(lhs.get()) < &(rhs.get());
		}
	};

	struct TypeEnvComparator {
		bool operator() (const std::reference_wrapper<const cola::VariableDeclaration>& lhs, const std::reference_wrapper<const cola::VariableDeclaration>& rhs) const {
			return &(lhs.get()) < &(rhs.get());
		}
	};


	using GuaranteeSet = std::set<std::reference_wrapper<const Guarantee>, GuaranteeSetComparator>;

	using TypeEnv = std::map<std::reference_wrapper<const cola::VariableDeclaration>, GuaranteeSet, TypeEnvComparator>;

} // namespace prtypes

#endif