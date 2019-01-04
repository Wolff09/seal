#include "types/check.hpp"
#include "types/inference.hpp"


bool prtypes::type_check(const cola::Program& /*program*/, std::set<std::reference_wrapper<const Guarantee>> /*guarantees*/) {
	throw std::logic_error("not yet implemented (prtypes::type_check)");
}