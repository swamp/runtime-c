cmake_minimum_required(VERSION 3.16.3)
project(runtime-example)
add_executable (runtime-example main.c)


target_link_libraries (runtime-example LINK_PUBLIC swamp-runtime)
