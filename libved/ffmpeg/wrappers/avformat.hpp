#pragma once

#include <tl/optional.hpp>
extern "C" {
#include <libavformat/avformat.h>
}

#include "avcodec.hpp"
#include "common.hpp"
#include <coroutine>
#include <cppcoro/generator.hpp>
#include <functional>
#include <span>

namespace libved::ffmpeg {
struct format_context_deleter {
  void operator()(AVFormatContext *c);
};

struct packet_ref {
  packet *pkt;
  packet_unref_guard guard;

  auto operator->() const { return pkt; }
  packet &operator*() const { return *this->pkt; }
};

class format_context
    : public std::unique_ptr<AVFormatContext, format_context_deleter> {
public:
  static constexpr std::size_t npos = static_cast<std::size_t>(-1);
  format_context(const char *input = nullptr);

  [[nodiscard]] std::span<stream *> streams() const noexcept;
  [[nodiscard]] std::tuple<std::size_t, const AVCodec *>
  find_stream(AVMediaType media_type, std::size_t wanted_stream_nb = npos,
              std::size_t related_stream_nb = npos, int flags = 0) const;

  [[nodiscard]] packet_unref_guard read_frame(AVPacket *pkt);

  [[nodiscard]] cppcoro::generator<packet_ref>
  read_frames(packet *pkt = nullptr) {
    packet pkt_ptr = nullptr;
    if (pkt == nullptr) {
      pkt_ptr = alloc_packet();
      pkt = &pkt_ptr;
    }
    while (true) {
      try {
        co_yield packet_ref{pkt, read_frame(pkt->get())};
      } catch (ffmpeg_error &err) {
        if (err.error_code() == AVERROR_EOF) {
          co_return;
        }

        throw;
      }
    }
  }
};
} // namespace libved::ffmpeg
