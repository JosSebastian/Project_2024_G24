cmake_minimum_required(VERSION 3.12)

find_package(MPI REQUIRED)
find_package(caliper REQUIRED)
find_package(adiak REQUIRED)

add_executable(merge_sort merge_sort.cpp)

message(STATUS "MPI includes : ${MPI_INCLUDE_PATH}")
message(STATUS "Caliper includes : ${caliper_INCLUDE_DIR}")
message(STATUS "Adiak includes : ${adiak_INCLUDE_DIRS}")
include_directories(SYSTEM ${MPI_INCLUDE_PATH})
include_directories(${caliper_INCLUDE_DIR})
include_directories(${adiak_INCLUDE_DIRS})

target_link_libraries(merge_sort PRIVATE MPI::MPI_CXX)
target_link_libraries(merge_sort PRIVATE caliper)
target_link_libraries(merge_sort PRIVATE adiak::adiak)
