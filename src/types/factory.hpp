#pragma once
#ifndef PRTYPES_FACTORY
#define PRTYPES_FACTORY

#include <memory>
#include "cola/ast.hpp"
#include "cola/observer.hpp"


namespace prtypes {

	std::unique_ptr<cola::Observer> make_base_smr_observer(const cola::Function& retire_function);
	

	std::unique_ptr<cola::Observer> make_hp_no_transfer_observer(const cola::Function& retire_function, const cola::Function& protect_function, std::string id=""); // TODO: optional unprotect

	std::unique_ptr<cola::Observer> make_hp_transfer_observer(const cola::Function& retire_function, const cola::Function& protect_function, const cola::Function& transfer_protect_function, std::string id=""); // TODO: optional unprotect


	std::unique_ptr<cola::Observer> make_ebr_observer(const cola::Function& retire_function, const cola::Function& enter_function, const cola::Function& leave_function, std::string id="");


	std::unique_ptr<cola::Observer> make_active_local_guarantee_observer(const cola::Function& retire_function, std::string prefix);

	std::unique_ptr<cola::Observer> make_last_free_observer(const cola::Program& program, std::string prefix="FF"); // observer accpeting *.free(*)


	std::vector<std::unique_ptr<cola::Observer>> make_hp_no_transfer_guarantee_observers(const cola::Function& retire_function, const cola::Function& protect_function, std::string id=""); // TODO: optional unprotect

	std::vector<std::unique_ptr<cola::Observer>> make_hp_transfer_guarantee_observers(const cola::Function& retire_function, const cola::Function& protect_function, const cola::Function& transfer_protect_function, std::string id=""); // TODO: optional unprotect

	std::vector<std::unique_ptr<cola::Observer>> make_ebr_guarantee_observers(const cola::Function& retire_function, const cola::Function& enter_function, const cola::Function& leave_function, std::string id="");

} // namespace prtypes

#endif