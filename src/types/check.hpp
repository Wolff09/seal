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
		Guarantee(std::string name_, std::unique_ptr<cola::Observer> observer_) : name(name_), observer(std::move(observer_)) {}
	};

	inline bool operator< (const std::reference_wrapper<const Guarantee>& lhs, const std::reference_wrapper<const Guarantee>& rhs){
		return &lhs < &rhs;
	}

	using GuaranteeSet = std::set<std::reference_wrapper<const Guarantee>>;

	using TypeEnv = std::map<std::reference_wrapper<const cola::VariableDeclaration>, GuaranteeSet>;

	bool type_check(const cola::Program& program, std::set<std::reference_wrapper<const Guarantee>> guarantees);

} // namespace prtypes

#endif