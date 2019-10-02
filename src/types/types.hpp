#pragma once
#ifndef PRTYPES_TYPES
#define PRTYPES_TYPES

#include <set>
#include <vector>
#include <memory>
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "types/check.hpp"
#include "types/sobserver.hpp"


namespace prtypes {

	struct TypeContext;

	struct Type {
		std::reference_wrapper<const TypeContext> context;
		SymbolicStateSet states;
		bool is_active;
		bool is_local;
		bool is_valid;
		bool is_transient;

		Type(const TypeContext& context, SymbolicStateSet states, bool is_active, bool is_local, bool is_valid, bool is_transient);
		Type(const TypeContext& context, SymbolicStateSet states, bool is_active, bool is_local, bool is_valid);
		Type(const TypeContext& context, SymbolicStateSet states, bool is_active, bool is_local);
	};

	struct TypeContext {
		const SmrObserverStore& observer_store;
		std::unique_ptr<SymbolicObserver> cross_product;
		const Type default_type, active_type, local_type, empty_type;

		TypeContext(const SmrObserverStore& store);
		TypeContext(const TypeContext&) = delete;
	};

	struct TypeEnvComparator {
		bool operator() (const std::reference_wrapper<const cola::VariableDeclaration>& lhs, const std::reference_wrapper<const cola::VariableDeclaration>& rhs) const {
			return &(lhs.get()) < &(rhs.get());
		}
	};

	using TypeEnv = std::map<std::reference_wrapper<const cola::VariableDeclaration>, Type, TypeEnvComparator>;


	bool equals(const Type& type, const Type& other);

	bool equals(const TypeEnv& env, const TypeEnv& other);

	Type type_union(const Type& type, const Type& other);

	Type type_intersection(const Type& type, const Type& other);

	TypeEnv type_intersection(const TypeEnv& env, const TypeEnv& other);

	Type type_closure(const Type& type);

	Type type_post(const Type& type, const cola::VariableDeclaration& variable, const cola::Command& command);

	TypeEnv type_post(const TypeEnv& env, const cola::Command& command);

	Type type_remove_local(const Type& type);

	Type type_remove_active(const Type& type);

	Type type_add_active(const Type& type);

} // namespace prtypes

#endif