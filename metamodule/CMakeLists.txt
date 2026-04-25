cmake_minimum_required(VERSION 3.22)

include($ENV{HOME}/metamodule-plugin-sdk/plugin.cmake)

project(Submit VERSION 1.0.0 DESCRIPTION "Submit plugin for MetaModule" LANGUAGES C CXX ASM)

add_library(Submit STATIC)

target_sources(Submit PRIVATE
    src/plugin.cpp
    src/Drift13.cpp
    src/Impact.cpp
    src/Chrono.cpp)

target_include_directories(Submit PRIVATE src)

create_plugin(
    SOURCE_LIB      Submit
    PLUGIN_NAME     Submit
    PLUGIN_JSON     $ENV{HOME}/Submit/plugin.json
    SOURCE_ASSETS   ${CMAKE_CURRENT_LIST_DIR}/assets
    DESTINATION     ${CMAKE_CURRENT_LIST_DIR}/metamodule-plugins
)
