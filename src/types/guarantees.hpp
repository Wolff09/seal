#pragma once
#ifndef PRTYPES_GUARANTEES
#define PRTYPES_GUARANTEES

#include <memory>
#include <string>
#include <set>
#include <map>
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "types/observer.hpp"


namespace prtypes {

	struct Guarantee final {
		std::string name;
		std::unique_ptr<cola::Observer> observer;
		bool is_transient = true; // false ==> stable under interference
		bool entails_validity = false; // true ==> pointer is guaranteed to be valid
		Guarantee(std::string name, std::unique_ptr<cola::Observer> observer) : name(name), observer(std::move(observer)) {
			assert(this->observer);
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
		const SmrObserverStore& observer_store;
		
		GuaranteeTable(const SmrObserverStore& observer_store);
		const Guarantee& active_guarantee() const;
		const Guarantee& local_guarantee() const;
		void add_guarantee(std::unique_ptr<cola::Observer> observer, std::string name);
		void add_guarantee(std::unique_ptr<cola::Observer> observer) {
			this->add_guarantee(std::move(observer), "G-" + std::to_string(this->all_guarantees.size()));
		}

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