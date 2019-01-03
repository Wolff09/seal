
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

message(STATUS "VATA2_INCLUDE_DIRS: ${VATA2_INCLUDE_DIRS}")
message(STATUS "VATA2_LIBS: ${VATA2_LIBS}")
message(STATUS "VATA2_LIB_STATIC: ${VATA2_LIB_STATIC}")


################################
####### setting up build #######
################################

set(SOURCES
	util.cpp
)

add_library(PRTypes ${SOURCES})
# add_dependencies(PRTypes CoLa)
target_link_libraries(PRTypes CoLa)