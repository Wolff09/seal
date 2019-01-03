# -*- mode:cmake -*-
# adapted from https://github.com/antlr/antlr4/blob/master/runtime/Cpp/cmake/ExternalAntlr4Cpp.cmake

set(VATA2_LOCAL_ROOT ${CMAKE_BINARY_DIR}/locals/vata2)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.2)
PROJECT(vata2_fetcher CXX)
INCLUDE(ExternalProject)

if(NOT EXISTS "${VATA2_REPO}")
  message(FATAL_ERROR "Unable to find VATA2 code base. VATA2_REPO: ${VATA2_REPO}")
endif()

# default path for source files
if (NOT VATA2_GENERATED_SRC_DIR)
  set(VATA2_GENERATED_SRC_DIR ${CMAKE_BINARY_DIR}/vata2_generated_src)
endif()

# build provided VATA2 version
ExternalProject_ADD(
  vata2
  PREFIX             ${VATA2_LOCAL_ROOT}
  URL                ${VATA2_REPO}
  CONFIGURE_COMMAND  ${CMAKE_COMMAND} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} ${CMAKE_FIND_ROOT_PATH} # this maybe added to force compilation with compiler specified to main CMake project
  CONFIGURE_COMMAND  ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_SOURCE_DIR:PATH=<SOURCE_DIR> <SOURCE_DIR>
  LOG_CONFIGURE ON
  LOG_BUILD ON
)

ExternalProject_Get_Property(vata2 INSTALL_DIR)
ExternalProject_Get_Property(vata2 SOURCE_DIR)

list(APPEND VATA2_INCLUDE_DIRS ${INSTALL_DIR}/include/)

set(VATA2_LIBS "${INSTALL_DIR}/lib")
set(VATA2_LIB_STATIC "${INSTALL_DIR}/lib/libvata2.a")
