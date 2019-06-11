
################################
####### setting up TCLAP #######
################################

set(TCLAP_REPO ${CMAKE_CURRENT_SOURCE_DIR}/tclap/tclap-1.2.2.tar.gz)

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tclap)
include(ExternalTclap)

include_directories(${TCLAP_INCLUDE_DIRS})

################################
####### setting up build #######
################################

# add_executable(Test main.cpp)
# target_link_libraries(Test CoLa PRTypes)

add_executable(leap leap.cpp)
target_link_libraries(leap CoLa PRTypes)
