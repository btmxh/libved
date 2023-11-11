#include "vaapi.hpp"
#include "ffmpeg/wrappers/common.hpp"
#include <cstdint>
#include <libdrm/drm_fourcc.h>
#include <va/va.h>
#include <va/va_drmcommon.h>
extern "C" {
#include <libavutil/buffer.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
}
#include <chrono>
#include <glad/egl.h>
#include <random>

namespace libved::vaapi {
nv12_texture::nv12_texture(const AVDRMFrameDescriptor &prime, int width,
                           int height) {
  const static std::array<uint32_t, 2> egl_formats{{
      DRM_FORMAT_R8,
      DRM_FORMAT_GR88,
  }};
  int image_index = 0;
  for (const auto &layer : std::span(prime.layers, prime.nb_layers)) {
    for (const auto &plane : std::span(layer.planes, layer.nb_planes)) {
      if (layer.format != egl_formats[image_index]) {
        throw std::runtime_error{"Wrong DRM format"};
      }

      auto tex_width = width / (image_index + 1);
      auto tex_height = height / (image_index + 1);

      // clang-format off
     EGLint img_attr[] = {
       EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(egl_formats[image_index]), 
       EGL_WIDTH, static_cast<EGLint>(tex_width),
       EGL_HEIGHT, static_cast<EGLint>(tex_height),
       EGL_DMA_BUF_PLANE0_FD_EXT, prime.objects[plane.object_index].fd,
       EGL_DMA_BUF_PLANE0_OFFSET_EXT, static_cast<EGLint>(plane.offset),
       EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(plane.pitch),
       EGL_NONE
     };
      // clang-format on

      m_images[image_index].reset(
          eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                            EGL_LINUX_DMA_BUF_EXT, nullptr, img_attr));
      if (m_images[image_index] == 0) {
        throw std::runtime_error{
            fmt::format("eglCreateImageKHR {}", image_index)};
      }

      m_textures[image_index] = staplegl::texture_2d{
          {},
          staplegl::resolution{static_cast<int32_t>(width),
                               static_cast<int32_t>(height)},
          {},
          {
              .min_filter = GL_LINEAR,
              .mag_filter = GL_LINEAR,
              .clamping = GL_CLAMP_TO_EDGE,
          },
          staplegl::tex_samples::MSAA_X1,
          false,
          false};
      m_textures[image_index].bind();
      glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_images[image_index].get());
      ++image_index;
    }
  }
}
} // namespace libved::vaapi
