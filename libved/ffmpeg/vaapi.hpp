#pragma once

#include "ffmpeg/wrappers/avcodec.hpp"
#include "ffmpeg/wrappers/avutil.hpp"
#include "glad/egl.h"
#include "staplegl.hpp"
#include <X11/Xlib.h>
#include <array>
#include <fmt/core.h>
#include <libavutil/buffer.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
#include <memory>
#include <stdexcept>
#include <tl/optional.hpp>
#include <variant>
namespace libved::vaapi {
inline constexpr auto egl_image_delete = [](EGLImage img) {
  eglDestroyImageKHR(eglGetCurrentDisplay(), img);
};

using egl_image = std::unique_ptr<std::remove_pointer_t<EGLImage>,
                                  decltype(egl_image_delete)>;
class nv12_texture {
public:
  nv12_texture(const AVDRMFrameDescriptor &prime, int width, int height);

  void bind_units(std::uint32_t luma, std::uint32_t chroma) {
    m_textures[0].set_unit(luma);
    m_textures[1].set_unit(chroma);
  }

private:
  std::array<egl_image, 2> m_images;
  std::array<staplegl::texture_2d, 2> m_textures;
};

using texture = std::variant<std::monostate, nv12_texture>;

class fd_guard {
public:
  fd_guard(tl::optional<int> fd = tl::nullopt) : m_fd{fd} {}
  ~fd_guard() {
    m_fd.map([](auto fd) { close(fd); });
  };

  void emplace(int fd) { m_fd.emplace(fd); }

  fd_guard(fd_guard &&rhs) : m_fd{std::exchange(rhs.m_fd, tl::nullopt)} {}
  fd_guard &operator=(fd_guard &&rhs) {
    m_fd.swap(rhs.m_fd);
    return *this;
  }
  fd_guard(const fd_guard &) = delete;
  fd_guard &operator=(const fd_guard &) = delete;

private:
  tl::optional<int> m_fd;
};

using drm_prime_guard = std::array<fd_guard, 4>;

struct guarded_texture {
  texture tex;
  drm_prime_guard fd_guards;
};

struct framed_texture {
  ffmpeg::frame frame;
  guarded_texture guarded_tex;
};

inline drm_prime_guard make_guard(const AVDRMFrameDescriptor &desc) {
  drm_prime_guard guards;
  for (int i = 0; i < desc.nb_objects; ++i) {
    guards[i].emplace(desc.objects[i].fd);
  }
  return guards;
}

inline guarded_texture map_nv12_frame(const ffmpeg::frame &hw_frame) {
  auto drm_frame = ffmpeg::alloc_frame();
  drm_frame->format = AV_PIX_FMT_DRM_PRIME;
  if (av_hwframe_map(drm_frame.get(), hw_frame.get(), 0)) {
    throw std::runtime_error("Couldn't map VAAPI hardware frame");
  }

  const auto &desc =
      *reinterpret_cast<const AVDRMFrameDescriptor *>(drm_frame->data[0]);
  auto nvtex = nv12_texture{desc, hw_frame->width, hw_frame->height};
  return guarded_texture{
      .tex = std::move(nvtex),
      .fd_guards = make_guard(desc),
  };
}
} // namespace libved::vaapi
