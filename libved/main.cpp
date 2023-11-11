#include "display.hpp"
#include "ffmpeg/vaapi.hpp"
#include "ffmpeg/wrappers/avcodec.hpp"
#include "ffmpeg/wrappers/avformat.hpp"
#include "ffmpeg/wrappers/avutil.hpp"
#include "ffmpeg/wrappers/common.hpp"
#include "vkfw/vkfw.hpp"
#include <chrono>
#include <cstdint>
#include <exception>
#include <fmt/core.h>
#include <fstream>
#include <glad/gles2.h>
#include <limits>
#include <ranges>
#include <spdlog/spdlog.h>
#include <staplegl.hpp>
#include <string>
#include <thread>
#include <tl/optional.hpp>

void print_exception(std::exception &ex, int rec_level = 0) {
  try {
    fmt::println(stderr, "{}exception: {}", std::string(rec_level, ' '),
                 ex.what());
    std::rethrow_if_nested(ex);
  } catch (std::exception &e) {
    print_exception(e, rec_level + 1);
  } catch (...) {
    fmt::println("Unknown exception");
  }
}

void packet_thread(const char* path) {
  try {
    auto dpl = libved::create_display(libved::display_params{
        .size = libved::extent2d{640, 360},
        .fps = std::numeric_limits<int>::max(),
        .window_title = "preview",
        .glfw_window_hints =
            {
                .clientAPI = vkfw::ClientAPI::eOpenGL_ES,
                .contextCreationAPI = vkfw::ContextCreationAPI::eEGL,
                .contextVersionMajor = 3u,
                .contextVersionMinor = 2u,
                .x11ClassName = "imgv",
                .x11InstanceName = "imgv",
            },
    });

    staplegl::shader_program const basic{"basic_shader",
                                         "./shaders/basic_shader.glsl"};
    staplegl::vertex_array vao;
    basic.bind();
    libved::ffmpeg::format_context fc{path};
    auto [video_stream_index, _] = fc.find_stream(AVMEDIA_TYPE_VIDEO);
    libved::ffmpeg::codec_context cc{*fc.streams()[video_stream_index]};
    auto hwtype = cc.create_hwdevice_auto();
    if (hwtype.has_value()) {
      spdlog::info("Using HW decoding: {}",
                   av_hwdevice_get_type_name(hwtype.value()));
      cc.set_format_callback([](auto...) { return AV_PIX_FMT_VAAPI; });
    }
    cc.init();
    auto vsi = video_stream_index;
    auto frame = libved::ffmpeg::alloc_frame();
    for (auto &&[pkt, guard] : fc.read_frames()) {
      if (pkt->get()->stream_index != vsi) {
        continue;
      }

      cc.send_packet(*pkt);
      while (cc.receive_frame(frame) ==
             libved::ffmpeg::send_receive_result::success) {
        auto nv12 = libved::vaapi::map_nv12_frame(std::move(frame));
        auto dpl_frame = dpl->new_frame();
        glClearColor(0.2F, 0.3F, 0.3F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        vao.bind();
        std::get<libved::vaapi::nv12_texture>(nv12.tex).bind_units(0, 1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        if(dpl->is_done()) {
          return;
        }
      }
    }
  } catch (std::exception &ex) {
    print_exception(ex);
  } catch (...) {
    fmt::println("Unknown exception");
  }
}

int main(int argc, char* argv[]) {
  packet_thread(argc > 1? argv[1] : "/home/torani/Videos/ortensia321.mkv");
  return 0;
}
