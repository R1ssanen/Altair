cmake_minimum_required(VERSION 4.0)
project(altair VERSION 1.0.0
               DESCRIPTION "Platform agnostic plugin framework library in pure C."
               LANGUAGES C)

add_library(${PROJECT_NAME} SHARED
   "src/altair/array.c"
   "src/altair/manager.c"
   "src/altair/plugin.c" 

   "src/altair/backend/windows/dll.c"
   "src/altair/backend/windows/log.c"
   "src/altair/backend/windows/threads.c"

   "src/altair/backend/unix/dll.c"
   "src/altair/backend/unix/log.c"
   "src/altair/backend/unix/threads.c"
   "src/altair/backend/unix/filewatcher.c"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_STANDARD 99
    C_VISIBILITY_PRESET "hidden"   
    VISIBILITY_INLINES_HIDDEN TRUE
    PUBLIC_HEADER "src/altair.h"
    EXPORT_COMPILE_COMMANDS TRUE
)

#target_include_directories(${PROJECT_NAME} PUBLIC "src/altair/")
target_include_directories(${PROJECT_NAME} PUBLIC "src/")
target_compile_definitions(${PROJECT_NAME} PRIVATE ALCORE)
target_link_libraries(${PROJECT_NAME} PRIVATE m)

# testing

add_executable(testing "testing/main.c")

target_link_libraries(testing PRIVATE ${PROJECT_NAME})
target_compile_definitions(testing PRIVATE ALCLIENT)

set_target_properties(testing PROPERTIES
    C_STANDARD 99   
    EXPORT_COMPILE_COMMANDS TRUE
)