
#list(APPEND EXTRA_COMPONENT_DIRS ../..)

idf_build_get_property(target IDF_TARGET)
set(priv_requires esp32-component-pubsub unity)

idf_component_register(
    SRC_DIRS "."
    PRIV_INCLUDE_DIRS "."
    PRIV_REQUIRES ${priv_requires} # driver test_utils
)

#-DCONFIG_UNITY_ENABLE_COLOR

#FILE(GLOB_RECURSE test_sources *.c *.cpp ../*.cpp)
#include(CMakePrintHelpers)
#cmake_print_variables(${test_sources})

#idf_component_register(SRCS ${test_sources}
#                       INCLUDE_DIRS "include")

#cmake_minimum_required(VERSION 3.15)

#include($ENV{IDF_PATH}/tools/cmake/project.cmake)

#list(APPEND EXTRA_COMPONENT_DIRS ../src)

#set(COMPONENTS untity EmbeddedPublishSubscribe main)

#project(TestEmbeddedPublishSubscribe)


#add_executable(MainProject ${test_sources})

#idf_component_register(SRCS ${test_sources}
#                       PRIV_INCLUDE_DIRS ../src
#                       PRIV_REQUIRES unity)

#include(CMakePrintHelpers)
#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    cmake_print_variables(${_variableName})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
