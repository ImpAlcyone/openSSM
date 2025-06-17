# src/CMakeLists.txt

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

foreach(file ${SRC_FILES})
    get_filename_component(name ${file} NAME_WE)
    add_library(${name} ${file})
    target_include_directories(${name} PUBLIC ${CMAKE_SOURCE_DIR}/include/src)
endforeach()
