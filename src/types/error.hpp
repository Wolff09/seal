#pragma once
#ifndef PRTYPES_ERROR
#define PRTYPES_ERROR

#include <exception>
#include <string>
#include "cola/ast.hpp"


namespace prtypes {

	struct TypeCheckError : public std::exception {
		const std::string cause;
		TypeCheckError(std::string cause_) : cause(std::move(cause_)) {}
		virtual const char* what() const noexcept { return cause.c_str(); }
	};

	struct UnsupportedConstructError : public TypeCheckError {
		UnsupportedConstructError(std::string cause) : TypeCheckError("Type check failed due to unsupported construct or expression: " + cause + ".") {}
	};

	struct UnsupportedObserverError : public TypeCheckError {
		UnsupportedObserverError(std::string cause) : TypeCheckError("Type check failed due to unsupported observer: " + cause + ".") {}
	};

	struct PointerRaceError : public TypeCheckError {
		PointerRaceError(std::string cause) : TypeCheckError("Type check failed tue to pointer race: " + cause + ".") {}
	};

	struct UnsafeDereferenceError : public PointerRaceError {
		const cola::Assignment& pc;
		const cola::Dereference& deref;
		const cola::VariableDeclaration& var;
		UnsafeDereferenceError(const cola::Assignment& pc_, const cola::Dereference& deref_, const cola::VariableDeclaration& var_) : PointerRaceError("unsafe dereference of potentially invalid variable " + var_.name), pc(pc_), deref(deref_), var(var_) {}
	};

	struct UnsafeAssumeError : public PointerRaceError {
		const cola::Assume & pc;
		const cola::VariableDeclaration& var;
		UnsafeAssumeError(const cola::Assume& pc_, const cola::VariableDeclaration& var_) : PointerRaceError("unsafe assume using potentially invalid variable " + var_.name), pc(pc_), var(var_) {}
	};

	struct UnsafeCallError : public PointerRaceError {
		const cola::Enter& pc;
		UnsafeCallError(const cola::Enter& pc_) : PointerRaceError("unsafe call of function " + pc_.decl.name), pc(pc_) {}
	};


	template<typename ErrorType, typename... ErrorTypeArgs>
	inline void raise_error(ErrorTypeArgs&&... args) {
		throw ErrorType(std::forward<ErrorTypeArgs>(args)...);
	}

	template<typename ErrorType, typename... ErrorTypeArgs>
	inline void conditionally_raise_error(bool condition, ErrorTypeArgs&&... args) {
		if (condition) {
			raise_error<ErrorType, ErrorTypeArgs&&...>(std::forward<ErrorTypeArgs>(args)...);
		}
	}

} // namespace prtypes

#endif