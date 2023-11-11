add_library(staplegl INTERFACE)
target_include_directories(staplegl INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")
add_library(staplegl::staplegl ALIAS staplegl)
