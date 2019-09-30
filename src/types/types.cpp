#include "types/types.hpp"

#include <iostream>
#include <list>

using namespace prtypes;
using cola::State;
using cola::Function;
using cola::Transition;
using cola::Observer;



//
// operations on observer states
//

inline SymbolicStateSet state_post(const SymbolicStateSet& set, const cola::Command& command, const cola::VariableDeclaration& variable) {
	return prtypes::symbolic_post(set, command, variable);
}

inline SymbolicStateSet state_closure(const SymbolicState& state) {
	return prtypes::symbolic_closure(state);
}

inline SymbolicStateSet state_closure(const SymbolicStateSet& set) {
	return prtypes::symbolic_closure(set);
}

inline SymbolicStateSet state_union(const SymbolicStateSet& set, const SymbolicStateSet& other) {
	SymbolicStateSet result(set);
	result.insert(other.begin(), other.end());
	return result;
}

inline SymbolicStateSet state_intersection(const SymbolicStateSet& set, const SymbolicStateSet& other) {
	SymbolicStateSet result;
	std::set_intersection(set.begin(), set.end(), other.begin(), other.end(), std::inserter(result, result.begin()));
	return result;
}

inline bool state_inclusion(const SymbolicStateSet& smaller, const SymbolicStateSet& bigger) {
	return std::includes(bigger.begin(), bigger.end(), smaller.begin(), smaller.end());
}


//
// types
//

inline bool compute_valid(const SymbolicStateSet& set) {
	// create a dummy "enter free(ptr)" to use state_post
	auto dummy_decl = std::make_unique<cola::VariableDeclaration>("dummy", Observer::free_function().args.at(0)->type, false);
	auto dummy_enter = std::make_unique<cola::Enter>(Observer::free_function());
	dummy_enter->args.push_back(std::make_unique<cola::VariableExpression>(*dummy_decl));

	// compute post and check if all post states are final
	auto free_post = state_post(set, *dummy_enter, *dummy_decl);
	for (const auto& state : free_post) {
		if (!state->is_final) {
			return false;
		}
	}
	return true;
}

inline bool compute_transient(const SymbolicStateSet& set) {
	return !state_inclusion(state_closure(set), set);
}

Type::Type(const TypeContext& context, SymbolicStateSet states, bool is_active, bool is_local, bool is_valid, bool is_transient)
	: context(context), states(states), is_active(is_active), is_local(is_local), is_valid(is_valid), is_transient(is_transient) {
}

Type::Type(const TypeContext& context, SymbolicStateSet states, bool is_active, bool is_local, bool is_valid) : Type(context, std::move(states), is_active, is_local, is_valid, false) {
	this->is_transient = compute_transient(this->states);
}

Type::Type(const TypeContext& context, SymbolicStateSet states, bool is_active, bool is_local) : Type(context, std::move(states), is_active, is_local, false, false) {
	this->is_valid = compute_valid(this->states);
	this->is_transient = compute_transient(this->states);
}


template<typename F>
inline Type make_type(const TypeContext& context, F filter) {
	SymbolicStateSet state_set;
	for (const auto& state : context.cross_product->states) {
		if (filter(*state)) {
			state_set.insert(state.get());
		}
	}
	return Type(context, std::move(state_set), false, false);
}

inline Type make_default_type(const TypeContext& context) {
	Type result = make_type(context, [](const SymbolicState& /*state*/) { return true; });
	return result;
}

inline Type make_active_local_type(const TypeContext& context, bool active) { // active = false ==> local
	Type result = make_type(context, [](const SymbolicState& state) { return state.is_active; });
	result.is_active = active;
	result.is_local = !active;
	if (!active) {
		result.is_transient = false;
	}
	return result;
}

inline Type make_empty_type(const TypeContext& context) {
	return Type(context, {}, false, false, true, false); // TODO: correct?
}

TypeContext::TypeContext(const SmrObserverStore& store)
	: observer_store(store),
	  cross_product(std::make_unique<SymbolicObserver>(store)),
	  default_type(make_default_type(*this)),
	  active_type(make_active_local_type(*this, true)),
	  local_type(make_active_local_type(*this, false)),
	  empty_type(make_empty_type(*this))
{}

//
// type operations
//

inline Type fix_type(const Type& type) {
	Type result(type);
	result.states = state_closure(type.states);
	result.is_transient = false;

	if (type.is_active) {
		result.states = state_intersection(result.states, type.context.get().active_type.states);
		result.is_transient |= type.context.get().active_type.is_transient;
	}
	if (type.is_local) {
		result.states = state_intersection(result.states, type.context.get().local_type.states);
		result.is_transient |= type.context.get().local_type.is_transient;
	}

	return result;
}

Type prtypes::type_union(const Type& type, const Type& other) {
	return fix_type(Type(
		type.context,
		state_intersection(type.states, other.states),
		type.is_active || other.is_active,
		type.is_local || other.is_local,
		type.is_valid || other.is_valid,
		type.is_transient || other.is_transient
	));
}

Type prtypes::type_intersection(const Type& type, const Type& other) {
	if (state_inclusion(type.states, other.states)) {
		return other;
	} else if (state_inclusion(other.states, type.states)) {
		return type;
	} else {
		return fix_type(Type(
			type.context,
			state_union(type.states, other.states),
			type.is_active && other.is_active,
			type.is_local && other.is_local,
			type.is_valid && other.is_valid,
			type.is_transient || other.is_transient
		));
	}
}

Type prtypes::type_closure(const Type& type) {
	auto closure = state_closure(type.states);
	bool valid = compute_valid(closure);
	return Type(type.context, std::move(closure), false, false, valid, false);
}

Type prtypes::type_post(const Type& type, const cola::VariableDeclaration& variable, const cola::Command& command) {
	// note: just doing post on states might give a set of locations that cannot be represented in the type system
	// compute post for states
	auto post = state_post(type.states, command, variable);

	// closure of post gives a valid type
	auto types_states = state_closure(post);

	// add active/local if allowed by the original post
	bool is_active = false;
	bool is_local = false;
	if (state_inclusion(post, type.context.get().active_type.states)) {
		is_active |= type.is_active;
		is_local |= type.is_local;
		if (is_active || is_local) {
			types_states = state_intersection(types_states, type.context.get().active_type.states);
		}
	}

	// construct type
	if (is_local) {
		return Type(type.context, std::move(types_states), is_active, is_local, true, false);
	}
	if (type.is_valid) {
		// compute validity (the pre type is valid => valid post type is allowed)
		return Type(type.context, std::move(types_states), is_active, is_local);

	} else {
		// resulting type must not be valid
		return Type(type.context, std::move(types_states), is_active, is_local, false);
	}
}

Type prtypes::type_remove_local(const Type& type) {
	Type result(type);
	result.is_local = false;
	return fix_type(result);
}

Type prtypes::type_remove_active(const Type& type) {
	Type result(type);
	result.is_active = false;
	return fix_type(result);
}

Type prtypes::type_add_active(const Type& type) {
	return type_union(type, type.context.get().active_type);
}

TypeEnv prtypes::type_intersection(const TypeEnv& env, const TypeEnv& other) {
	TypeEnv result;
	for (const auto& [decl, type] : env) {
		auto find = other.find(decl);
		if (find != other.end()) {
			result.insert({ decl, type_intersection(type, find->second) });
		}
	}
	return result;
}

TypeEnv prtypes::type_post(const TypeEnv& env, const cola::Command& command) {
	TypeEnv result(env);
	for (auto& [decl, type] : result) {
		type = type_post(type, decl, command);
	}
	return result;
}


bool prtypes::equals(const Type& type, const Type& other) {
	return type.is_local == other.is_local
	    && type.is_active == other.is_active
	    && type.is_valid == other.is_valid
	    && type.is_transient == other.is_transient
	    && type.states == other.states
	    && &type.context.get() == &other.context.get()
	    ;
}

bool prtypes::equals(const TypeEnv& env, const TypeEnv& other) {
	if (env.size() != other.size()) {
		return false;
	} else {
		auto env_it = env.begin();
		auto other_it = other.begin();
		while (env_it != env.end()) {
			assert(other_it != other.end());
			if (!prtypes::equals(env_it->second, other_it->second)) {
				return false;
			}
			++env_it;
			++other_it;
		}
		return true;
	}
}
