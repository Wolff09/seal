#pragma once
#ifndef PRTYPES_ERROR
#define PRTYPES_ERROR


namespace prtypes {

	inline void raise_unsupported(std::string cause) {
		throw std::logic_error("Type check failed: " + cause + ".");
	}

	inline void raise_unsupported_if(bool condition, std::string cause) {
		if (!condition) {
			raise_unsupported(cause);
		}
	}

	inline void raise_type_error(std::string cause) {
		throw std::logic_error("Type error: " + cause + ".");
	}

	inline void raise_type_error_if(bool condition, std::string cause) {
		if (!condition) {
			raise_type_error(cause);
		}
	}

} // namespace prtypes

#endif