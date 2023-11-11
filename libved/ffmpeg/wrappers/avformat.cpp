#include "avformat.hpp"
#include "common.hpp"
#include <cstddef>
#include <limits>
#include <new>
#include <tuple>
#include <utility>

namespace libved::ffmpeg {
inline auto int_to_size_t(int x, std::size_t negative = format_context::npos)
    -> std::size_t {
  return x < 0 ? negative : static_cast<int>(x);
}

inline auto size_t_to_int(std::size_t x, int overflow = -1) -> std::size_t {
  return x > static_cast<std::size_t>(std::numeric_limits<int>::max())
             ? overflow
             : x;
}

void format_context_deleter::operator()(AVFormatContext *c) {
  avformat_close_input(&c);
}

format_context::format_context(const char *input) {
  AVFormatContext *c = nullptr;
  call_and_handle_error(
      throw_nested_runtime_error("Unable to open input at url '{}'", input),
      avformat_open_input, &c, input, nullptr, nullptr);
  reset(c);

  call_and_handle_error(
      throw_nested_runtime_error("Unable to find stream info"),
      avformat_find_stream_info, c, nullptr);
}

[[nodiscard]] std::span<AVStream *> format_context::streams() const noexcept {
  return std::span{get()->streams, get()->nb_streams};
}

std::tuple<std::size_t, const AVCodec *>
format_context::find_stream(AVMediaType media_type,
                            std::size_t wanted_stream_nb,
                            std::size_t related_stream_nb, int flags) const {
  const AVCodec *codec = nullptr;
  const auto index =
      av_find_best_stream(get(), media_type, size_t_to_int(wanted_stream_nb),
                          size_t_to_int(related_stream_nb), &codec, flags);
  return std::make_tuple(int_to_size_t(index), codec);
}

packet_unref_guard format_context::read_frame(AVPacket *pkt) {
  call_and_handle_error(no_throw_nested(), av_read_frame, get(), pkt);
  return packet_unref_guard{pkt};
}

} // namespace libved::ffmpeg
