#pragma once
#ifndef COMMON_CROSSPRODUCT
#define COMMON_CROSSPRODUCT


#include <memory>
// #include <vector>
// #include <map>
#include "cola/ast.hpp"
#include "cola/observer.hpp"


namespace common {

	std::unique_ptr<cola::Observer> cross_product(std::vector<std::reference_wrapper<const cola::Observer>> observers);

}

#endif