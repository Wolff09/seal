#pragma once
#ifndef PRTYPES_CHECK
#define PRTYPES_CHECK

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

	// inline bool operator< (const std::reference_wrapper<const Guarantee>& lhs, const std::reference_wrapper<const Guarantee>& rhs){
	// 	return &(lhs.get()) < &(rhs.get());
	// }

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

	bool type_check(const cola::Program& program, std::set<std::reference_wrapper<const Guarantee>> guarantees);

	void test();

} // namespace prtypes

#endif