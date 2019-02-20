
################################
####### setting up vata2 #######
################################

set(VATA2_REPO ${CMAKE_CURRENT_SOURCE_DIR}/vata2/libvata2_custom.zip)

# load Vata2 lib/includes
LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vata2)
include(ExternalVata2)

# add Vata2 path
include_directories(${VATA2_INCLUDE_DIRS})
link_directories(${VATA2_LIBS})



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
	inference.cpp
	guarantees.cpp
	checker_accept.cpp
	checker_check.cpp
	check.cpp
	simulation.cpp
	observer.cpp
	factory.cpp
	cave.cpp
	rmraces.cpp
)

add_library(PRTypes ${SOURCES})
# add_dependencies(PRTypes CoLa)
target_link_libraries(PRTypes CoLa ${VATA2_LIB_STATIC} ${Z3_LIBRARY})