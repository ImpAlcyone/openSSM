cmake_minimum_required(VERSION 3.14)
project(Subaru_ECU_util C)

set(CMAKE_C_STANDARD 17)

# Output binary in bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include/src
    ${CMAKE_SOURCE_DIR}/include/ui
)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(ui)

# Main executable (depends on needed shared libs)
add_executable(Subaru_ECU_util main.c)

# Link desired modules here
target_link_libraries(Subaru_ECU_util
    config
    dump
    error_scan
    log
    measure
    ssm
    dump_ui
    error_ui
    main_ui
    measure_ui
)
