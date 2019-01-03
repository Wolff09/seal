# -*- mode:cmake -*-
# adapted from https://github.com/antlr/antlr4/blob/master/runtime/Cpp/cmake/ExternalAntlr4Cpp.cmake

set(ANTLR4CPP_LOCAL_ROOT ${CMAKE_BINARY_DIR}/locals/antlr4cpp)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12.2)
PROJECT(antlr4cpp_fetcher CXX)
INCLUDE(ExternalProject)

# only JRE required
FIND_PACKAGE(Java COMPONENTS Runtime REQUIRED)

if(NOT EXISTS "${ANTLR_JAR}")
  message(FATAL_ERROR "Unable to find antlr tool. ANTLR_JAR: ${ANTLR_JAR}")
endif()

if(NOT EXISTS "${ANTLR_REPO}")
  message(FATAL_ERROR "Unable to find antlr code base. ANTLR_REPO: ${ANTLR_REPO}")
endif()

# default path for source files
if (NOT ANTLR4CPP_GENERATED_SRC_DIR)
  set(ANTLR4CPP_GENERATED_SRC_DIR ${CMAKE_BINARY_DIR}/antlr4cpp_generated_src)
endif()

# build provided antlr version
ExternalProject_ADD(
  antlr4cpp
  PREFIX             ${ANTLR4CPP_LOCAL_ROOT}
  URL                ${ANTLR_REPO}
  CONFIGURE_COMMAND  ${CMAKE_COMMAND} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} ${CMAKE_FIND_ROOT_PATH} # this maybe added to force compilation with compiler specified to main CMake project
  CONFIGURE_COMMAND  ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -DANTLR4CPP_JAR_LOCATION=${ANTLR_JAR} -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_SOURCE_DIR:PATH=<SOURCE_DIR>/runtime/Cpp <SOURCE_DIR>/runtime/Cpp
  LOG_CONFIGURE ON
  LOG_BUILD ON
)

ExternalProject_Get_Property(antlr4cpp INSTALL_DIR)

list(APPEND ANTLR4CPP_INCLUDE_DIRS ${INSTALL_DIR}/include/antlr4-runtime)
foreach(src_path misc atn dfa tree support)
  list(APPEND ANTLR4CPP_INCLUDE_DIRS ${INSTALL_DIR}/include/antlr4-runtime/${src_path})
endforeach(src_path)

set(ANTLR4CPP_LIBS "${INSTALL_DIR}/lib")
set(ANTLR4CPP_LIB_SHARED "${INSTALL_DIR}/lib/libantlr4-runtime.so")
set(ANTLR4CPP_LIB_STATIC "${INSTALL_DIR}/lib/libantlr4-runtime.a")


############ Generate runtime #################
# macro to add dependencies to target
#
# Param 1 project name
# Param 1 namespace (postfix for dependencies)
# Param 2 Lexer file (full path)
# Param 3 Parser File (full path)
#
# output
#
# antlr4cpp_src_files_{namespace} - src files for add_executable
# antlr4cpp_include_dirs_{namespace} - include dir for generated headers
# antlr4cpp_generation_{namespace} - for add_dependencies tracking

macro(antlr4cpp_process_grammar
    antlr4cpp_project
    antlr4cpp_project_namespace
    antlr4cpp_grammar_lexer
    antlr4cpp_grammar_parser)

  if(EXISTS "${ANTLR_JAR}")
    message(STATUS "Found antlr tool: ${ANTLR_JAR}")
  else()
    message(FATAL_ERROR "Unable to find antlr tool. ANTLR4CPP_JAR_LOCATION:${ANTLR_JAR}")
  endif()

  add_custom_target("antlr4cpp_generation_${antlr4cpp_project_namespace}"
    COMMAND
    ${CMAKE_COMMAND} -E make_directory ${ANTLR4CPP_GENERATED_SRC_DIR}
    COMMAND
    "${Java_JAVA_EXECUTABLE}" -jar "${ANTLR_JAR}" -Werror -Dlanguage=Cpp -listener -visitor -o "${ANTLR4CPP_GENERATED_SRC_DIR}/${antlr4cpp_project_namespace}" -package ${antlr4cpp_project_namespace} "${antlr4cpp_grammar_lexer}" "${antlr4cpp_grammar_parser}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS "${antlr4cpp_grammar_lexer}" "${antlr4cpp_grammar_parser}"
    )

  # Find all the input files
  FILE(GLOB generated_files ${ANTLR4CPP_GENERATED_SRC_DIR}/${antlr4cpp_project_namespace}/*.cpp)

  # export generated cpp files into list
  foreach(generated_file ${generated_files})
    list(APPEND antlr4cpp_src_files_${antlr4cpp_project_namespace} ${generated_file})
    set_source_files_properties(
      ${generated_file}
      PROPERTIES
      COMPILE_FLAGS -Wno-overloaded-virtual
      )
  endforeach(generated_file)
  message(STATUS "Antlr4Cpp  ${antlr4cpp_project_namespace} Generated: ${generated_files}")

  # export generated include directory
  set(antlr4cpp_include_dirs_${antlr4cpp_project_namespace} ${ANTLR4CPP_GENERATED_SRC_DIR}/${antlr4cpp_project_namespace})
  message(STATUS "Antlr4Cpp ${antlr4cpp_project_namespace} include: ${ANTLR4CPP_GENERATED_SRC_DIR}/${antlr4cpp_project_namespace}")

endmacro()
