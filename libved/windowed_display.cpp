#include "windowed_display.hpp"
#include "vkfw/vkfw.hpp"
#include <GLFW/glfw3.h>
#include <glad/egl.h>
#include <glad/gles2.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <tl/optional.hpp>
#include <type_traits>
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_EGL
#include <GLFW/glfw3native.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

namespace libved::windowed {
window_frame::window_frame(window &owner) : m_owner(owner) {}

window_frame::~window_frame() {
  m_owner.m_window->swapBuffers();
  m_owner.m_current_frame = tl::nullopt;
}

static void set_x11_window_mode(GLFWwindow *window) {
  auto *const x11_display = glfwGetX11Display();
  const auto x11_window = glfwGetX11Window(window);
  const auto window_type = XInternAtom(x11_display, "_NET_WM_WINDOW_TYPE", 0);
  const auto splash = XInternAtom(x11_display, "_NET_WM_WINDOW_TYPE_SPLASH", 0);
  XChangeProperty(x11_display, x11_window, window_type, XA_ATOM, 32,
                  PropModeReplace,
                  reinterpret_cast<_Xconst unsigned char *>(&splash), 1);
}

window::window(const display_params &params) : m_inst{vkfw::initUnique()} {
  auto [width, height] =
      params.size
          .or_else([] {
            spdlog::warn("No render size specified, defaulting to 640x360");
            return extent2d{640, 360};
          })
          .value();
  auto default_fps = vkfw::getPrimaryMonitor().getVideoMode()->refreshRate;
  auto fps = params.fps
                 .or_else([=] {
                   spdlog::warn("No render FPS specified, defaulting to {}fps",
                                default_fps);
                   return default_fps;
                 })
                 .value();
  auto title = params.window_title.value_or("Preview");
  auto hints = params.glfw_window_hints;
  hints.visible = false;
  m_window = vkfw::createWindowUnique(width, height, title, hints);
  set_x11_window_mode(*m_window);
  m_window->makeContextCurrent();
  if (!gladLoadGLES2(vkfw::getProcAddress)) {
    throw std::runtime_error{"Unable to load OpenGL functions"};
  }
  if (!gladLoadEGL(glfwGetEGLDisplay(), vkfw::getProcAddress)) {
    throw std::runtime_error{"Unable to load OpenGL functions"};
  }
  if (fps > default_fps) {
    spdlog::warn(
        "FPS higher than display refresh rate. This will be "
        "interpreted as disabling VSync. Variable FPS will be supported in the "
        "near future (display refresh rate: {}, fps: {})",
        default_fps, fps);
    vkfw::swapInterval(0);
  } else if (default_fps % fps == 0) {
    vkfw::swapInterval(default_fps / fps);
  } else {
    spdlog::error("FPS value {} not supported. Variable FPS will be supported "
                  "in the near future",
                  fps);
    vkfw::swapInterval(1);
  }
  m_window->setAspectRatio(width, height);
  m_window->callbacks()->on_framebuffer_resize = [](auto, std::size_t width,
                                                    std::size_t height) {
    glViewport(0, 0, static_cast<GLint>(width), static_cast<GLint>(height));
  };
  m_window->show();
}

bool window::is_rendering() const { return false; }
bool window::is_done() const { return m_window->shouldClose(); }

extent2d window::framebuffer_size() const {
  const auto [width, height] = m_window->getFramebufferSize();
  return {width, height};
}

std::unique_ptr<display_frame> window::new_frame() {
  vkfw::pollEvents();
  return std::make_unique<window_frame>(*this);
}
} // namespace libved::windowed
