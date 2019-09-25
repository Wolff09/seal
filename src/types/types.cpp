#include "types/types.hpp"


using namespace prtypes;
using cola::State;
using cola::Function;
using cola::Transition;
using cola::Observer;


//
// combining states
//
template<typename T> // T = container<const State*>
inline StateVecSet combine_states(std::vector<T> list) {
	std::vector<typename T::const_iterator> begin, cur, end;
	for (std::size_t index = 0; index < list.size(); ++index) {
		begin.push_back(list.at(index).begin());
		cur.push_back(list.at(index).begin());
		end.push_back(list.at(index).end());
	}

	StateVecSet result;
	while (cur != end) {
		// get current vector
		StateVec element;
		for (const auto& it : cur) {
			element.push_back(*it);
		}
		result.insert(element);

		// progress cur
		for (std::size_t index = 0; index < cur.size(); ++index) {
			cur.at(index)++;

			if (cur.at(index) == end.at(index)) {
				cur.at(index) = begin.at(index);
			} else {
				break;
			}
		}
	}

	return result;
}


//
// post for observed thread
//
// struct PostVisitor : public cola::ObserverVisitor {
// 	void visit(const cola::ThreadObserverVariable& /*obj*/) override { throw std::logic_error("Unexpected invocation (PostVisitor::visit(const ThreadObserverVariable&)"); }
// 	void visit(const cola::ProgramObserverVariable& /*obj*/) override { throw std::logic_error("Unexpected invocation (PostVisitor::visit(const ProgramObserverVariable&)"); }
// 	void visit(const cola::SelfGuardVariable& /*obj*/) override { throw std::logic_error("Unexpected invocation (PostVisitor::visit(const SelfGuardVariable&)"); }
// 	void visit(const cola::ArgumentGuardVariable& /*obj*/) override { throw std::logic_error("Unexpected invocation (PostVisitor::visit(const ArgumentGuardVariable&)"); }
// 	void visit(const cola::TrueGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation (PostVisitor::visit(const TrueGuard&)"); }
// 	void visit(const cola::ConjunctionGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation (PostVisitor::visit(const ConjunctionGuard&)"); }
// 	void visit(const cola::EqGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation (PostVisitor::visit(const EqGuard&)"); }
// 	void visit(const cola::NeqGuard& /*obj*/) override { throw std::logic_error("Unexpected invocation (PostVisitor::visit(const NeqGuard&)"); }
// 	void visit(const Transition& /*obj*/) override { throw std::logic_error("Unexpected invocation (PostVisitor::visit(const Transition&)"); }
// 	void visit(const Observer& /*obj*/) override { throw std::logic_error("Unexpected invocation (PostVisitor::visit(const Observer&)"); }


// 	void visit(const State& /*obj*/) override { throw std::logic_error("Unexpected invocation (PostVisitor::visit(const State&)"); }
// };

// struct PostInfo {
// 	enum Observation { OBSERVED, UNOBSERVED, ANY };
// 	const Function& label;
// 	Transition::Kind kind;
// 	Observation thread_observation;
// 	Observation address_observation;
// 	PostInfo(const Function& label, Transition::Kind kind, Observation thread_observation, Observation address_observation)
// 		: label(label), kind(kind), thread_observation(thread_observation), address_observation(address_observation) {}
// 	PostInfo(const Function& label, Transition::Kind kind) : PostInfo(label, kind, ANY, ANY) {}
// };

// inline StateVecSet general_post(const StateVecSet& /*set*/, PostInfo /*info*/) {
// 	throw std::logic_error("not yet implemented (general_post)");
// }


//
// operations on observer states
//

inline bool state_final(const StateVec& vec) {
	for (const auto& state : vec) {
		if (state->final) {
			return true;
		}
	}
	return false;
}

inline std::set<const State*> state_post(const State* /*state*/, const cola::VariableDeclaration& /*variable*/, const cola::Command& /*command*/) {
	// TODO [requires State to know outgoing transitions]
	throw std::logic_error("not yet implemented (state_post)");
}

inline StateVecSet state_post(const StateVec& vec, const cola::VariableDeclaration& variable, const cola::Command& command) {
	// compute per state post
	std::vector<std::set<const State*>> post(vec.size());
	for (std::size_t index = 0; index < vec.size(); ++index) {
		post.at(index) = state_post(vec.at(index), variable, command);
	}

	return combine_states(post);
}

inline StateVecSet state_post(const StateVecSet& set, const cola::VariableDeclaration& variable, const cola::Command& command) {
	StateVecSet result;
	for (const auto& vec : set) {
		StateVecSet post = state_post(vec, variable, command);
		result.insert(post.begin(), post.end());
	}
	return result;
}

inline StateVecSet state_closure(const StateVecSet& /*set*/) {
	// TODO [requires State to know outgoing transitions]
	throw std::logic_error("not yet implemented (state_closure)");
}

inline StateVecSet state_union(const StateVecSet& set, const StateVecSet& other) {
	StateVecSet result(set);
	result.insert(other.begin(), other.end());
	return result;
}

inline StateVecSet state_intersection(const StateVecSet& set, const StateVecSet& other) {
	StateVecSet result;
	std::set_intersection(set.begin(), set.end(), other.begin(), other.end(), std::inserter(result, result.begin()));
	return result;
}

inline bool state_inclusion(const StateVecSet& smaller, const StateVecSet& bigger) {
	return std::includes(bigger.begin(), bigger.end(), smaller.begin(), smaller.end());
}


//
// types
//

inline bool compute_valid(const StateVecSet& set) {
	// create a dummy "enter free(ptr)" to use state_post
	auto dummy_decl = std::make_unique<cola::VariableDeclaration>("dummy", Observer::free_function().args.at(0)->type, false);
	auto dummy_enter = std::make_unique<cola::Enter>(Observer::free_function());
	dummy_enter->args.push_back(std::make_unique<cola::VariableExpression>(*dummy_decl));

	// compute post and check if all post states are final
	auto free_post = state_post(set, *dummy_decl, *dummy_enter);
	for (const auto& vec : free_post) {
		if (!state_final(vec)) {
			return false;
		}
	}
	return true;
}

inline bool compute_transient(const StateVecSet& set) {
	return !state_inclusion(state_closure(set), set);
}

Type::Type(const TypeContext& context, StateVecSet states, bool is_active, bool is_local, bool is_valid, bool is_transient)
	: context(context), states(states), is_active(is_active), is_local(is_local), is_valid(is_valid), is_transient(is_transient) {
}

Type::Type(const TypeContext& context, StateVecSet states, bool is_active, bool is_local, bool is_valid) : Type(context, std::move(states), is_active, is_local, is_valid, compute_transient(states)) {
}

Type::Type(const TypeContext& context, StateVecSet states, bool is_active, bool is_local) : Type(context, std::move(states), is_active, is_local, compute_valid(states), compute_transient(states)) {
}


template<typename F>
inline Type make_type(const TypeContext& context, const SmrObserverStore& store, F filter) {
	std::vector<std::vector<const State*>> list(store.impl_observer.size() + 1);
	auto add_states = [&list, &filter] (const Observer& observer, std::size_t index) {
		auto& vector = list.at(index);
		vector.reserve(observer.states.size());
		for (const auto& state : observer.states) {
			if (filter(*state, index)) {
				vector.push_back(state.get());
			}
		}
	};

	add_states(*store.base_observer, 0);
	for (std::size_t index = 0; index < store.impl_observer.size(); ++index) {
		add_states(*store.impl_observer.at(index), index + 1);
	}

	return Type(context, combine_states(list), false, false);
}

inline Type make_default_type(const TypeContext& context, const SmrObserverStore& store) {
	return make_type(context, store, [](const State& /*state*/, std::size_t /*index*/) { return true; });
}

inline Type make_active_local_type(const TypeContext& context, const SmrObserverStore& store) {
	return make_type(context, store, [](const State& state, std::size_t index) { return index != 0 || state.initial; });
}

inline Type make_empty_type(const TypeContext& context, const SmrObserverStore& /*store*/) {
	return Type(context, {}, false, false, false, false);
}

TypeContext::TypeContext(const SmrObserverStore& store)
	: observer_store(store),
	default_type(make_default_type(*this, store)),
	active_type(make_active_local_type(*this, store)),
	local_type(make_active_local_type(*this, store)),
	empty_type(make_empty_type(*this, store))
{}


//
// type operations
//

Type prtypes::type_union(const Type& type, const Type& other) {
	return Type(
		type.context,
		state_intersection(type.states, other.states),
		type.is_active || other.is_active,
		type.is_local || other.is_local,
		type.is_valid || other.is_valid,
		type.is_transient || other.is_transient
	);
}

Type prtypes::type_intersection(const Type& /*type*/, const Type& /*other*/) {
	// TODO: implement (union of states)
	throw std::logic_error("not yet implement (type_intersection");
}

Type prtypes::type_closure(const Type& type) {
	auto closure = state_closure(type.states);
	bool valid = compute_valid(closure);
	return Type(type.context, std::move(closure), false, false, valid, false);
}

Type prtypes::type_post(const Type& type, const cola::VariableDeclaration& variable, const cola::Command& command) {
	// note: just doing post on states might give a set of locations that cannot be represented in the type system
	// compute post for states
	auto post = state_post(type.states, variable, command);

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
	if (type.is_valid) {
		// compute validity (the pre type is valid => valid post type is allowed)
		return Type(type.context, std::move(types_states), is_active, is_local);
	} else {
		// resulting type must not be valid
		return Type(type.context, std::move(types_states), is_active, is_local, false);
	}
}

inline void fix_type(Type& /*type*/) {
	// TODO: add closure if needed?
	throw std::logic_error("not yet implement (type_fix");
}

Type prtypes::type_remove_local(const Type& type) {
	Type result(type);
	result.is_local = false;
	fix_type(result);
	return result;
}

Type prtypes::type_remove_active(const Type& type) {
	Type result(type);
	result.is_active = false;
	fix_type(result);
	return result;
}


