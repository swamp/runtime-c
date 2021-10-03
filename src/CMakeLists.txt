cmake_minimum_required(VERSION 3.16.3)
project(swamp-runtime C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_COMPILER_NAMES clang)
# set( CMAKE_VERBOSE_MAKEFILE on )

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(isDebug TRUE)
else()
    set(isDebug FALSE)
endif()

message("isDebug: ${isDebug}")

set(useSanitizers ${isDebug})
message("useSanitizers: ${useSanitizers}")

if (useSanitizers)
    message("using sanitizers")
    set(sanitizers "-fsanitize=address")
endif()

if (APPLE)
        add_compile_definitions(TORNADO_OS_MACOS)
        set(OS_MACOS 1)
elseif (UNIX)
        add_compile_definitions(TORNADO_OS_LINUX)
        set(OS_LINUX 1)
endif()

if (isDebug)
        message("Debug build detected")
        set(CONFIGURATION_DEBUG 1)
        add_compile_definitions(CONFIGURATION_DEBUG)
endif()

set(deps ../deps/)

add_compile_definitions(_POSIX_C_SOURCE=200112L)
add_compile_options(-Wall -Wextra -Wshadow -Weffc++ -Wstrict-aliasing -ansi -pedantic -Wno-unused-function -Wno-unused-parameter)




file(GLOB_RECURSE lib_src FOLLOW_SYMLINKS
        "lib/*.c"
        )

message("deps ${deps}")

file(GLOB_RECURSE deps_src FOLLOW_SYMLINKS
        "${deps}*/src/lib/*.c"
        "${deps}swamp/*/src/lib/*.c"
        )

message("deps_src ${deps_src}")
function(unixlike)
        message("unixlike")
endfunction()

if (OS_LINUX)
        message("Linux Detected!")
        unixlike()

elseif (OS_MACOS)
        message("MacOS detected!")
        unixlike()
endif()


add_library(swamp-runtime
        ${lib_src}
        ${deps_src}
)
target_include_directories(swamp-runtime PUBLIC include)
target_include_directories(swamp-runtime PUBLIC ${deps}swamp/dump-c/src/include)
target_include_directories(swamp-runtime PUBLIC ${deps}swamp/typeinfo-c/src/include)
target_include_directories(swamp-runtime PUBLIC ${deps}clog/src/include)
target_include_directories(swamp-runtime PUBLIC ${deps}tiny-libc/src/include)
target_include_directories(swamp-runtime PUBLIC ${deps}flood-c/src/include)
target_include_directories(swamp-runtime PUBLIC ${deps}raff-c/src/include)

target_include_directories(swamp-runtime PUBLIC ${deps}swamp/ecs-c/src/include) # remove this later
target_include_directories(swamp-runtime PUBLIC ${deps}swamp/ecs-wrap-c/src/include) # remove this later
target_include_directories(swamp-runtime PUBLIC ${deps}tinge-c/src/include) # remove this later
target_include_directories(swamp-runtime PUBLIC ${deps}swamp/yaml-load-c/src/include) # remove this later
target_include_directories(swamp-runtime PUBLIC ${deps}monotonic-time-c/src/include) # remove this later

set_target_properties(swamp-runtime
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)


if (useSanitizers)
    target_link_libraries(swamp-runtime ${sanitizers})
endif()


target_compile_options(swamp-runtime PRIVATE -Wall -Wno-unused-variable ${sanitizers})

if (NOT isDebug)
    message("optimize!")
    target_compile_options(swamp-runtime PRIVATE -O3) #  -flto file format no recognized
endif()

target_link_libraries(swamp-runtime m)

