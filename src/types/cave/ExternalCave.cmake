# -*- mode:cmake -*-
# adapted from https://github.com/antlr/antlr4/blob/master/runtime/Cpp/cmake/ExternalAntlr4Cpp.cmake

set(CAVE_LOCAL_ROOT ${CMAKE_BINARY_DIR}/locals/cave)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.2)
PROJECT(cave_fetcher CXX)
INCLUDE(ExternalProject)

if(NOT EXISTS "${CAVE_REPO}")
  message(FATAL_ERROR "Unable to find CAVE code base. CAVE_REPO: ${CAVE_REPO}")
endif()

# default path for source files
if (NOT CAVE_GENERATED_SRC_DIR)
  set(CAVE_GENERATED_SRC_DIR ${CMAKE_BINARY_DIR}/cave_generated_src)
endif()

# build provided VATA2 version
ExternalProject_ADD(
  cave
  PREFIX             ${CAVE_LOCAL_ROOT}
  URL                ${CAVE_REPO}
  SOURCE_DIR         ${CAVE_LOCAL_ROOT}/src/cave
  BUILD_IN_SOURCE    TRUE
  CONFIGURE_COMMAND  ""
  BUILD_COMMAND      "make"
  INSTALL_COMMAND    cp ${CAVE_LOCAL_ROOT}/src/cave/cave ${CMAKE_BINARY_DIR}/bin
  LOG_CONFIGURE ON
  LOG_BUILD ON
)

set(CAVE_EXECUTABLE ${CMAKE_BINARY_DIR}/bin/cave)
