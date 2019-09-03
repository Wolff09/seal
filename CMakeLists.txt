# project(seal CXX)
set(TOOL_NAME seal)

# built requirements
cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # set default release type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# remain in testing mode for the moment
# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE RelWithDebInfo)
# set(CMAKE_VERBOSE_MAKEFILE ON)

# configure compiler
add_definitions(-fno-strict-aliasing)
add_definitions(-Wall -Wextra)
add_definitions(-fshow-column)
# add_definitions(-Wno-unused-parameter -Wno-unused-function)

# adds test target
# enable_testing()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# includes
include_directories(src)

# sources/executables
add_subdirectory(src)
add_subdirectory(test)
