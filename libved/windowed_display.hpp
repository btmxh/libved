#pragma once

#include "display.hpp"
#include <memory>
#include <tl/optional.hpp>
#include <vkfw/vkfw.hpp>

namespace libved::windowed {
class window;
class window_frame : public display_frame {
public:
  window_frame(window& owner);
  ~window_frame();

private:
  window &m_owner;
};
class window : public display {
public:
  window(const display_params &params);
  ~window() override = default;
  bool is_rendering() const override;
  bool is_done() const override;

  extent2d framebuffer_size() const override;
  std::unique_ptr<display_frame> new_frame() override;

private:
  vkfw::UniqueInstance m_inst;
  vkfw::UniqueWindow m_window;
  friend class window_frame;
  tl::optional<window_frame &> m_current_frame;
};
} // namespace libved::windowed
