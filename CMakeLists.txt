cmake_minimum_required(VERSION 4.0)
project(altair VERSION 1.0.0
               DESCRIPTION "Platform agnostic plugin framework library in pure C."
               LANGUAGES C)

set (CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set (LIBALTAIR ${PROJECT_NAME})

add_library(${LIBALTAIR} SHARED
   "src/altair/array.c"
   "src/altair/manager.c"
   "src/altair/plugin.c"
   "src/altair/string.c"

   "src/altair/backend/windows/dll.c"
   "src/altair/backend/windows/log.c"
   "src/altair/backend/windows/threads.c"

   "src/altair/backend/unix/dll.c"
   "src/altair/backend/unix/log.c"
   "src/altair/backend/unix/threads.c"
   "src/altair/backend/unix/filewatcher.c"
)

set_target_properties(${LIBALTAIR} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_STANDARD 99
    C_VISIBILITY_PRESET "hidden"   
    VISIBILITY_INLINES_HIDDEN TRUE
    PUBLIC_HEADER "src/altair.h"
)

target_include_directories(${LIBALTAIR} PUBLIC "${CMAKE_SOURCE_DIR}/src/")
target_compile_definitions(${LIBALTAIR} PRIVATE ALCORE)
target_link_libraries(${LIBALTAIR} PRIVATE m)

# runtime

add_executable(runtime "runtime.c")

set_target_properties(runtime PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(runtime PRIVATE ${LIBALTAIR})
target_compile_definitions(runtime PRIVATE ALCLIENT)

# plugins

add_subdirectory("plugins/keyboard")
