
################################
####### setting up antlr #######
################################

set(ANTLR_JAR ${CMAKE_CURRENT_SOURCE_DIR}/parser/antlr/antlr-4.7.2-complete.jar)
set(ANTLR_REPO ${CMAKE_CURRENT_SOURCE_DIR}/parser/antlr/antlr4-master.zip)
set(ANTLR_GRAMMAR ${CMAKE_CURRENT_SOURCE_DIR}/parser/CoLa.g4)
set(ANTLR_NAMESPACE "cola")

# load ANTLR jar/lib/includes
LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/parser/antlr)
include(ExternalAntlr4Cpp)

# add ANTLR path
include_directories(${ANTLR4CPP_INCLUDE_DIRS})
link_directories(${ANTLR4CPP_LIBS})

# manually add files (needed to avoid recompilation for unchanged grammar)
set(generated_files
	${ANTLR4CPP_GENERATED_SRC_DIR}/CoLaBaseListener.cpp
	${ANTLR4CPP_GENERATED_SRC_DIR}/CoLaBaseVisitor.cpp
	${ANTLR4CPP_GENERATED_SRC_DIR}/CoLaListener.cpp
	${ANTLR4CPP_GENERATED_SRC_DIR}/CoLaVisitor.cpp
	${ANTLR4CPP_GENERATED_SRC_DIR}/CoLaLexer.cpp
	${ANTLR4CPP_GENERATED_SRC_DIR}/CoLaParser.cpp
)

# generate lexer/parser from grammar
add_custom_command(
	OUTPUT ${generated_files}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${ANTLR4CPP_GENERATED_SRC_DIR}
	COMMAND "${Java_JAVA_EXECUTABLE}" -jar "${ANTLR_JAR}" -Werror -Dlanguage=Cpp -listener -visitor -o "${ANTLR4CPP_GENERATED_SRC_DIR}" -package ${ANTLR_NAMESPACE} "${ANTLR_GRAMMAR}"
	# COMMAND touch ${generated_files} # force update to avoid spurious recompilation
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	DEPENDS "${ANTLR_GRAMMAR}"
)

# let CMake know that the files are generated during build
foreach(generated_file ${generated_files})
	list(APPEND antlr4cpp_src_files ${generated_file})
	set_source_files_properties(
		${generated_file}
		PROPERTIES
		COMPILE_FLAGS -Wno-overloaded-virtual
		GENERATED TRUE
	)
endforeach(generated_file)

# add path for generated files
set(antlr4cpp_include_dirs ${ANTLR4CPP_GENERATED_SRC_DIR})
include_directories(${antlr4cpp_include_dirs})


################################
####### setting up build #######
################################

set(SOURCES
	# parser
	parser/AstBuilder.cpp
	parser/ObserverBuilder.cpp
	parser/parse.cpp

	# transform
	transform/rmCAS.cpp
	transform/rmConditionals.cpp
	transform/rmJumps.cpp
	transform/rmScopedVars.cpp
	transform/rmUselessScopes.cpp
	transform/simplifyExpr.cpp

	# util
	util/cpExpr.cpp
	util/cpStmt.cpp
	util/cpObserver.cpp
	util/negExpr.cpp
	util/print.cpp
)

add_library(CoLa ${antlr4cpp_src_files} ${SOURCES})
add_dependencies(CoLa antlr4cpp)
target_link_libraries(CoLa ${ANTLR4CPP_LIB_STATIC})
