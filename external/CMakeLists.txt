add_library(vkfw INTERFACE)
target_include_directories(vkfw INTERFACE "${CMAKE_CURRENT_LIST_DIR}/vkfw/include")
add_library(vkfw::vkfw ALIAS vkfw)
target_compile_definitions(vkfw INTERFACE VKFW_NO_STRUCT_CONSTRUCTORS)

add_subdirectory(glad/cmake)
glad_add_library(glad REPRODUCIBLE
  API egl=1.5 gles2=3.2
  EXTENSIONS
    GL_OES_EGL_image
    EGL_KHR_image_base
    EGL_EXT_image_dma_buf_import)
add_library(glad::glad ALIAS glad)
