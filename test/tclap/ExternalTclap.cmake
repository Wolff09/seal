# -*- mode:cmake -*-
# adapted from https://github.com/antlr/antlr4/blob/master/runtime/Cpp/cmake/ExternalAntlr4Cpp.cmake

set(TCLAP_LOCAL_ROOT ${CMAKE_BINARY_DIR}/locals/tclap)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.2)
PROJECT(tclap_fetcher CXX)
INCLUDE(ExternalProject)

if(NOT EXISTS "${TCLAP_REPO}")
  message(FATAL_ERROR "Unable to find TCLAP code base. TCLAP_REPO: ${TCLAP_REPO}")
endif()

# default path for source files
if (NOT TCLAP_GENERATED_SRC_DIR)
  set(TCLAP_GENERATED_SRC_DIR ${CMAKE_BINARY_DIR}/tclap_generated_src)
endif()

# build provided TCLAP version
ExternalProject_ADD(
  tclap
  PREFIX             ${TCLAP_LOCAL_ROOT}
  URL                ${TCLAP_REPO}
  SOURCE_DIR         ${TCLAP_LOCAL_ROOT}/tclap
  BUILD_IN_SOURCE    TRUE
  CONFIGURE_COMMAND  ""
  BUILD_COMMAND      ""
  INSTALL_COMMAND    ""
  LOG_CONFIGURE ON
  LOG_BUILD ON
)

ExternalProject_Get_Property(tclap SOURCE_DIR)
list(APPEND TCLAP_INCLUDE_DIRS ${SOURCE_DIR}/include/)

