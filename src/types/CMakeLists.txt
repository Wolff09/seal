
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
####### setting up build #######
################################

set(SOURCES
	util.cpp
)

add_library(PRTypes ${SOURCES})
# add_dependencies(PRTypes CoLa)
target_link_libraries(PRTypes CoLa)