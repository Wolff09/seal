
################################
####### setting up TCLAP #######
################################

set(TCLAP_REPO ${CMAKE_CURRENT_SOURCE_DIR}/tclap/tclap-1.2.2.tar.gz)

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tclap)
include(ExternalTclap)

add_library(TCLAP INTERFACE)
target_include_directories(TCLAP INTERFACE ${TCLAP_INCLUDE_DIRS})

################################
####### setting up build #######
################################

add_executable(${TOOL_NAME} Main.cpp)
target_link_libraries(${TOOL_NAME} CoLa PRTypes TCLAP)


################################
######### installation #########
################################

install(TARGETS ${TOOL_NAME} DESTINATION ${INSTALL_FOLDER})