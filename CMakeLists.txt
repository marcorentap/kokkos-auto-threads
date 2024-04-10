cmake_minimum_required(VERSION 3.16)
project(simplesim LANGUAGES CXX)
set(CMAKE_BUILD_TYPE "DEBUG")

find_package(Kokkos REQUIRED)

add_executable(kokkosautothreads Executor.cpp Analyzer.cpp KokkosAutoThreads.cpp)
target_link_libraries(kokkosautothreads PRIVATE Kokkos::kokkos)
target_compile_features(kokkosautothreads PRIVATE cxx_std_17)

add_library(kokkosautothreads_lib SHARED LibKokkosAutoThreads.cpp)
set_target_properties(kokkosautothreads_lib PROPERTIES OUTPUT_NAME kokkosautothreads)

install(TARGETS kokkosautothreads kokkosautothreads_lib)
