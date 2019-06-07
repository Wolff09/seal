
set(SOURCES
	cross_product.cpp

	# TODO: move observer factory here
)

add_library(Common ${SOURCES})
add_dependencies(Common CoLa)
# target_link_libraries(CoLa ${ANTLR4CPP_LIB_STATIC})
