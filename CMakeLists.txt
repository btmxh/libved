cmake_minimum_required(VERSION 3.26)

project(libved LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

file(GLOB_RECURSE libved_SOURCES CONFIGURE_DEPENDS "libved/*.hpp" "libved/*.cpp")
add_executable(libved ${libved_SOURCES})
target_include_directories(libved PUBLIC libved)

find_package(sol2 REQUIRED)
find_package(FFmpeg COMPONENTS AVCODEC AVFORMAT AVUTIL REQUIRED)
find_package(fmt REQUIRED)
find_package(tl-optional REQUIRED)
find_package(cppcoro REQUIRED)
find_package(OpenGL COMPONENTS EGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenAL REQUIRED)
find_package(spdlog REQUIRED)
find_package(X11 REQUIRED)
add_subdirectory(staplegl)
add_subdirectory(external)

target_link_libraries(libved PUBLIC FFmpeg::AVCODEC FFmpeg::AVFORMAT FFmpeg::AVUTIL sol2 fmt tl::optional cppcoro::cppcoro OpenGL::EGL glfw glad::glad staplegl::staplegl vkfw::vkfw spdlog::spdlog X11::X11)
target_compile_definitions(libved PUBLIC __STDC_CONSTANT_MACROS)

