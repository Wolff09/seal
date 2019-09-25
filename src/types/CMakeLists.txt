
################################
####### setting up CAVE ########
################################

set(CAVE_REPO ${CMAKE_CURRENT_SOURCE_DIR}/cave/cave-2.1.zip)

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cave)
include(ExternalCave)


################################
######## setting up z3 #########
################################

# TODO: check version
find_library(Z3_LIBRARY NAMES z3 REQUIRED)
find_path(Z3_INCLUDE NAMES "z3++.h" REQUIRED)
include_directories(${Z3_INCLUDE})


################################
####### setting up build #######
################################

set(SOURCES
	checker_accept.cpp
	checker_check.cpp
	check.cpp
	simulation.cpp
	observer.cpp
	factory.cpp
	cave.cpp
	rmraces.cpp
	preprocess.cpp
	types.cpp
)

add_library(PRTypes ${SOURCES})
# add_dependencies(PRTypes CoLa)
target_link_libraries(PRTypes CoLa ${Z3_LIBRARY})


################################
######### installation #########
################################

install(PROGRAMS ${CAVE_EXECUTABLE} DESTINATION ${INSTALL_FOLDER})
