cmake_minimum_required(VERSION 4.0)
project(keyboard)

add_library(${PROJECT_NAME} SHARED "keyboard.c")
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 99
    C_VISIBILITY_PRESET "hidden"   
    VISIBILITY_INLINES_HIDDEN TRUE
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins/keyboard/"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE ALPLUGIN ALCLIENT)
find_library(LIBALTAIR altair "../../x64/")

target_link_libraries(keyboard PRIVATE ${LIBALTAIR} ncurses)
target_include_directories(keyboard PRIVATE "../../src/")
