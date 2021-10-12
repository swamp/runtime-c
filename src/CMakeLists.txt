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
elseif (WIN32)
        add_compile_definitions(TORNADO_OS_WINDOWS)
        set(OS_WINDOWS 1)
        message("windows detected")
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
if (!OS_WINDOWS)
add_compile_options(-Wall -Wextra -Wshadow -Weffc++ -Wstrict-aliasing -ansi -pedantic -Wno-unused-function -Wno-unused-parameter -Wall -Wno-unused-variable)
endif()


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
        file(GLOB_RECURSE deps_platform_src FOLLOW_SYMLINKS
                "${deps}*/src/platform/posix/*.c"
                )
        message("deps_platform_src ${deps_platform_src}")
elseif (OS_MACOS)
        message("MacOS detected!")
        unixlike()
endif()


add_library(swamp-runtime
        ${lib_src}
        ${deps_src}
        ${deps_platform_src}
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
target_include_directories(swamp-runtime PUBLIC ${deps}impact-swamp/src/include) # remove this later
target_include_directories(swamp-runtime PUBLIC ${deps}impact/src/include) # remove this later
target_include_directories(swamp-runtime PUBLIC ${deps}basal-c/src/include) # remove this later
target_include_directories(swamp-runtime PUBLIC ${deps}tyran/src/include) # remove this later

set_target_properties(swamp-runtime
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)


if (useSanitizers)
    target_link_libraries(swamp-runtime ${sanitizers})
endif()


target_compile_options(swamp-runtime PRIVATE  ${sanitizers})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    message("optimize!")
    target_compile_options(swamp-runtime PRIVATE -O3) #  -flto file format no recognized
else()
    target_compile_options(swamp-runtime PRIVATE -O0 -g)
endif()

target_link_libraries(swamp-runtime m)

