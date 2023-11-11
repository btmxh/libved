#pragma once

#include "vkfw/vkfw.hpp"
#include <cstddef>
#include <memory>
#include <tl/optional.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace libved {

struct extent2d {
  std::size_t width;
  std::size_t height;
};

struct display_params {
  tl::optional<extent2d> size;
  tl::optional<int> fps;
  tl::optional<const char *> window_title;
  vkfw::WindowHints glfw_window_hints{
      .clientAPI = vkfw::ClientAPI::eOpenGL,
      .contextCreationAPI = vkfw::ContextCreationAPI::eEGL,
  };
};

class display_frame {
public:
  virtual ~display_frame() = default;
};

class display {
public:
  display();
  virtual ~display() = default;

  virtual bool is_rendering() const = 0;
  virtual bool is_done() const = 0;

  virtual extent2d framebuffer_size() const = 0;
  virtual std::unique_ptr<display_frame> new_frame() = 0;

private:
};

std::unique_ptr<display> create_display(const display_params &params,
                                        bool render = false);
} // namespace libved
