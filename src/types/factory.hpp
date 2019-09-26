#pragma once
#ifndef PRTYPES_FACTORY
#define PRTYPES_FACTORY

#include <memory>
#include "cola/ast.hpp"
#include "cola/observer.hpp"


namespace prtypes {

	std::unique_ptr<cola::Observer> make_base_smr_observer(const cola::Function& retire_function);

} // namespace prtypes

#endif