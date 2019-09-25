#pragma once
#ifndef PRTYPES_TYPES
#define PRTYPES_TYPES

#include <set>
#include <vector>
#include "cola/ast.hpp"
#include "cola/observer.hpp"
#include "types/observer.hpp"


namespace prtypes {

	using StateVec = std::vector<const cola::State*>;
	using StateVecSet = std::set<StateVec>;

	struct TypeContext;

	struct Type {
		const TypeContext& context;
		StateVecSet states;
		bool is_active;
		bool is_local;
		bool is_valid;
		bool is_transient;

		Type(const TypeContext& context, StateVecSet states, bool is_active, bool is_local, bool is_valid, bool is_transient);
		Type(const TypeContext& context, StateVecSet states, bool is_active, bool is_local, bool is_valid);
		Type(const TypeContext& context, StateVecSet states, bool is_active, bool is_local);
	};

	struct TypeContext {
		const SmrObserverStore& observer_store;
		const Type default_type, active_type, local_type;

		TypeContext(const SmrObserverStore& store);
		TypeContext(const TypeContext&) = delete;
	};


	Type type_union(const Type& type, const Type& other);

	Type type_closure(const Type& type);

	Type type_post(const Type& type, const cola::VariableDeclaration& variable, const cola::Command& command);

} // namespace prtypes

#endif