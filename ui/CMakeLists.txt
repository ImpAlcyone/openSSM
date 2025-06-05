file(GLOB UI_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

foreach(file ${UI_FILES})
    get_filename_component(name ${file} NAME_WE)
    add_library(${name} SHARED ${file})
    target_include_directories(${name} PUBLIC ${CMAKE_SOURCE_DIR}/include/ui)
endforeach()
