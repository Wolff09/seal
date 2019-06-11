#include "common/cross_product.hpp"

#include <set>
#include <map>
#include <vector>
#include <deque>

using namespace common;
using namespace cola;


struct CrossProductHelper final {
	std::vector<std::reference_wrapper<const Observer>> observers;
	std::vector<const Transition*> all_transitions;

	std::map<const State*, std::vector<const State*>> cross2states;
	std::map<std::vector<const State*>, const State*> states2cross;
	std::deque<State*> worklist;
	std::unique_ptr<Observer> result;

	CrossProductHelper(std::vector<std::reference_wrapper<const Observer>> observers) : observers(std::move(observers)) {
		for (const Observer& observer : this->observers) {
			for (const auto& transition : observer.transitions) {
				all_transitions.push_back(transition.get());
			}
		}

		// TODO: we assume here that the observers are all negative specs?
		// TODO: we assume here that the observers are all deterministic?
	}

	void add_state(const std::vector<const State*>& states) {
		assert(states.size() == observers.size());
		auto find = states2cross.find(states);
		if (find == states2cross.end()) {
			result->states.push_back(std::make_unique<State>());
			State* new_state = result->states.back().get();
			cross2states.insert({ new_state, states });
			states2cross.insert({ states, new_state });
			worklist.push_back(new_state);
		}
	}

	std::vector<const State*> get_initial_states() {
		// TODO: ensure each observer has exactly one initial state
		std::vector<const State*> result;
		result.reserve(observers.size());
		for (const Observer& observer : observers) {
			bool found_initial = false;
			for (const auto& state : observer.states) {
				if (state->initial) {
					assert(!found_initial);
					result.push_back(state.get());
				}
			}
			assert(found_initial);
		}
		assert(result.size() == observers.size());
		return result;
	}


	std::string make_state_name(const std::vector<const State*>& states) {
		std::string result;
		bool first = true;
		for (const State* state : states) {
			if (!first) {
				result += " x ";
			} else {
				first = false;
			}
			result += state->name;
		}
		return result;
	}

	bool is_final(const std::vector<const State*>& states) {
		for (const State* state : states) {
			if (state->final) {
				return true;
			}
		}
		return false;
	}

	bool is_outgoing(const std::vector<const State*>& states, const Transition& transition) {
		for (const State* state : states) {
			if (state == &transition.src) {
				return true;
			}
		}
		return false;
	}

	std::set<std::pair<const Function*, Transition::Kind>> collect_outgoing_labels(const std::vector<const State*>& states) {
		std::set<std::pair<const Function*, Transition::Kind>> result;
		for (const Transition* transition : all_transitions) {
			if (is_outgoing(states, *transition)) {
				result.insert({ &transition->label, transition->kind });
			}
		}
		return result;
	}

	std::vector<std::deque<const Transition*>> collect_outgoing_transitions_per_state(const std::vector<const State*>& states, const Function& label, Transition::Kind kind) {
		std::vector<std::deque<const Transition*>> result(states.size());
		std::map<const State*, std::deque<const Transition*>&> state2trans;
		for (std::size_t index = 0; index < states.size(); ++index) {
			state2trans.insert({ states.at(index), result.at(index) });
		}
		for (const Transition* transition : all_transitions) {
			if (&transition->label == &label && transition->kind == kind && is_outgoing(states, *transition)) {
				state2trans.at(&transition->src).push_back(transition);
			}
		}
		return result;
	}

	std::set<std::vector<const Transition*>> get_all_transition_cominations(const std::vector<std::deque<const Transition*>>& collection) {
		std::set<std::vector<const Transition*>> result;

		using iterator_t = std::deque<const Transition*>::const_iterator;
		std::vector<iterator_t> iterators;
		std::vector<iterator_t> begin;
		std::vector<iterator_t> end;
		for (const auto& deque : collection) {
			iterators.push_back(deque.cbegin());
			begin.push_back(deque.cbegin());
			end.push_back(deque.cend());
		}
		assert(iterators.size() == collection.size());
		assert(begin.size() == collection.size());
		assert(end.size() == collection.size());

		auto increment = [&]() {
			for (std::size_t index = 0; index < iterators.size(); ++index) {
				if (iterators.at(index) == end.at(index)) {
					continue;
				}
				++iterators.at(index);
				if (iterators.at(index) != end.at(index)) {
					break;
				} else {
					iterators.at(index) = begin.at(index);
				}
			}	
		};
		auto is_done = [&]() -> bool {
			for (std::size_t index = 0; index < iterators.size(); ++index) {
				if (iterators.at(index) != end.at(index)) {
					return false;
				}
			}
			return true;
		};

		for (; !is_done(); increment()) {
			std::vector<const Transition*> combination;
			for (std::size_t index = 0; index < iterators.size(); ++index) {
				if (iterators.at(index) != end.at(index)) {
					combination.push_back(*iterators.at(index));
				} else {
					combination.push_back(nullptr);
				}
			}
			assert(combination.size() == iterators.size());
			result.insert(std::move(combination));
		}

		return result;
	}

	const State& make_post_state(const std::vector<const State*>& states, const std::vector<const Transition*>& transitions) {
		assert(states.size() == transitions.size());
		std::vector<const State*> new_state(states);
		for (std::size_t index = 0; index < states.size(); ++index) {
			if (transitions.at(index)) {
				new_state.at(index) = &transitions.at(index)->dst;
			}
		}
		add_state(new_state);
		return *states2cross.at(new_state);
	}

	std::unique_ptr<Guard> translate_guard(const Guard& guard) {
		throw std::logic_error("not implemented");
	}

	std::unique_ptr<Guard> translate_and_merge_guards(const std::vector<const Transition*>& transitions) {
		throw std::logic_error("not implemented");
	}

	std::deque<std::vector<bool>> make_boolean_combinations(std::size_t size) {
		assert(size > 0);
		std::deque<std::vector<bool>> result;
		std::vector<bool> current(size);

		auto all_set = [](const std::vector<bool>& vec) -> bool {
			for (bool b : vec) {
				if (!b) {
					return false;
				}
			}
			return true;
		};
		auto increment = [&]() {
			for (std::size_t index = 0; index < current.size(); ++index) {
				if (current.at(index)) {
					current.at(index) = false;
				} else {
					current.at(index) = true;
					break;
				}
			}
		};

		for (; !all_set(current); increment()) {
			increment();
			std::vector<bool> copy(current);
			result.push_back(std::move(current));
		}
		result.push_back(std::move(current));
		return result;
	}

	std::deque<std::unique_ptr<Guard>> make_guards(const std::vector<const Transition*>& transitions) {
		// for (const auto& bcombination : make_boolean_combinations(transitions.size())) {
		// 	auto guard = translate_and_merge_guardsfvl;jkdjhfidlahfjklsdahjfjkl;dsah;fljdsfdsaklfj;dasfijflo;rihjfiklo;dsjlkfdjsa l;kfdsjl;kafds
		// }
		throw std::logic_error("not yet implemented");
	}

	void create_transitions(const State& crossstate, const std::vector<const State*>& states) {
		auto out_labels = collect_outgoing_labels(states);
		for (auto [label, kind] : out_labels) {
			auto transition_collection = collect_outgoing_transitions_per_state(states, *label, kind);
			auto transition_combinations = get_all_transition_cominations(transition_collection);
			for (const auto& combination : transition_combinations) {
				const State& post = make_post_state(states, combination);
				auto guard_combinations = make_guards(combination);
				for (auto& guard : guard_combinations) {
					auto transition = std::make_unique<Transition>(crossstate, post, *label, kind, std::move(guard));
					result->transitions.push_back(std::move(transition));
				}
			}
		}
	}


	void init() {
		result = std::make_unique<Observer>();
		// TODO: observer variables
	}

	void build() {
		init();
		while (!worklist.empty()) {
			State& state = *worklist.front();
			worklist.pop_front();

			auto& repr = cross2states.at(&state);
			state.name = make_state_name(repr);
			state.final = is_final(repr);

			create_transitions(state, repr);
		}
	}


	std::unique_ptr<Observer> get() {
		build();
		return std::move(result);
	}
};


std::unique_ptr<Observer> common::cross_product(std::vector<std::reference_wrapper<const Observer>> observers) {
	return CrossProductHelper(std::move(observers)).get();
}
