#pragma once
#ifndef PRTYPES_ERROR
#define PRTYPES_ERROR

#include <exception>
#include <string>
#include "cola/ast.hpp"


namespace prtypes {

	struct CaveError : public std::exception {
		const std::string cause;
		CaveError(std::string cause_) : cause(std::move(cause_)) {}
		virtual const char* what() const noexcept { return cause.c_str(); }
	};


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
		virtual ~PointerRaceError() = default;
		enum Kind { DEREF, ASSUME, CALL };
		PointerRaceError(std::string cause) : TypeCheckError("Type check failed due to pointer race: " + cause + ".") {}
		virtual Kind kind() const = 0;
	};

	struct UnsafeDereferenceError : public PointerRaceError {
		const cola::Assignment& pc;
		const cola::Dereference& deref;
		const cola::VariableDeclaration& var;
		UnsafeDereferenceError(const cola::Assignment& pc_, const cola::Dereference& deref_, const cola::VariableDeclaration& var_) : PointerRaceError("unsafe dereference (" + std::to_string(pc_.id) + ") of potentially invalid variable " + var_.name), pc(pc_), deref(deref_), var(var_) {}
		virtual Kind kind() const override { return DEREF; }
	};

	struct UnsafeAssumeError : public PointerRaceError {
		const cola::Assume& pc;
		const cola::VariableDeclaration& var;
		UnsafeAssumeError(const cola::Assume& pc_, const cola::VariableDeclaration& var_) : PointerRaceError("unsafe assume (" + std::to_string(pc_.id) + ") using potentially invalid variable " + var_.name), pc(pc_), var(var_) {}
		virtual Kind kind() const override { return ASSUME; }
	};

	struct UnsafeCallError : public PointerRaceError {
		const cola::Enter& pc;
		UnsafeCallError(const cola::Enter& pc_) : PointerRaceError("unsafe call of function " + pc_.decl.name), pc(pc_) {}
		UnsafeCallError(const cola::Enter& pc_, std::string cause) : PointerRaceError("unsafe call (" + std::to_string(pc_.id) + ") of function " + pc_.decl.name + " (" + cause + ")"), pc(pc_) {}
		virtual Kind kind() const override { return CALL; }
	};

	struct RefinementError : public TypeCheckError {
		RefinementError(std::string cause) : TypeCheckError("Resolving pointer race failed: " + cause + ".") {}
	};


	struct TypeSynthesisError : public std::exception {
		const std::string cause;
		TypeSynthesisError(std::string cause_) : cause("Synthesis failed: " + std::move(cause_) + ".") {}
		virtual const char* what() const noexcept { return cause.c_str(); }
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