#pragma once
#ifndef PRTYPES_CONFIG
#define PRTYPES_CONFIG


namespace prtypes {

	// does not infer invalid guarantees if a valid guarantee has already been inferred
	#define INFERENCE_SKIP_GUARANTEES_IF_ALREADY_VALID true

	// prune guarantees that are included in some valid guarantee
	#define SYNTHESIS_PRUNE_GUARANTEES_BY_INCLUSION true

} // namespace prtypes

#endif