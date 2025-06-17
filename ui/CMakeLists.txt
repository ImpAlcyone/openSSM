# ui/CMakeLists.txt

file(GLOB UI_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(ui STATIC ${UI_FILES})

target_include_directories(ui PUBLIC ${CMAKE_SOURCE_DIR}/include/ui)
target_link_libraries(ui ncurses)
