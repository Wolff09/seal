#pragma once
#ifndef COLA_SIMPLIFY
#define COLA_SIMPLIFY


#include <memory>
#include <vector>
#include <map>
#include <string>
#include "cola/ast.hpp"


namespace cola {

	// TODO: loop peeling -> should be done before remove_conditionals and remove_jumps

	/** Removes variable declarations from non-top-level scopes.
	  * As a result, the given program has only shared and function local variables.
	  */
	void remove_scoped_variables(Program& program);

	/** Replaces if and while with choose and loop.
	  */
	void remove_conditionals(Program& program);

	/** Removes continue and break statments.
	  */
	void remove_jumps(Program& program);

	/** Flattens scopes of scopes into scopes.
	  */
	void remove_useless_scopes(Program& program);

	/** Rewrites expressions into simpler normal form.
	  */
	void simplify_expressions(Program& program);

	/** Replaces CAS with atomic blocks.
	  * Affects only those CAS that appear on their own or as the right-hand side of assignments.
	  * (Does not affect CAS nested in expressions.)
	  */
	void remove_cas(Program& program);

	/** Rewrites program to CoLa Light.
	  */
	inline void simplify(Program& program) {
		// Note: removing conditionals should be done only after jump removal
		// Reason: while(true) relies on breaks; but jump removal would add a assume(false)
		remove_jumps(program);
		remove_conditionals(program);
		remove_scoped_variables(program);
		simplify_expressions(program);
		remove_cas(program);
		remove_useless_scopes(program);
	}

} // namespace cola

#endif