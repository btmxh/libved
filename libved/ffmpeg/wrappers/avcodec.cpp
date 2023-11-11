#include "avcodec.hpp"
#include "ffmpeg/wrappers/common.hpp"
#include <errors.hpp>
#include <libavcodec/avcodec.h>
#include <new>
#include <spdlog/spdlog.h>
#include <unordered_set>

namespace libved::ffmpeg {
template <typename T> inline tl::optional<T &> optional_ref_from_ptr(T *ptr) {
  return ptr == nullptr ? tl::nullopt : tl::optional<T &>{*ptr};
}

packet alloc_packet() { return call_alloc(no_throw_nested(), av_packet_alloc); }

packet::packet(AVPacket *p) : std::unique_ptr<AVPacket, packet_deleter>{p} {}
void packet_deleter::operator()(AVPacket *p) { av_packet_free(&p); }
packet_unref_guard::packet_unref_guard(AVPacket *f) : m_packet{f} {}
packet_unref_guard::packet_unref_guard(const packet &f)
    : packet_unref_guard{f.get()} {}
packet_unref_guard::~packet_unref_guard() {
  if (m_packet != nullptr) {
    av_packet_unref(m_packet);
  }
}

packet_unref_guard::packet_unref_guard(packet_unref_guard &&other)
    : m_packet{std::exchange(other.m_packet, nullptr)} {}

packet_unref_guard &packet_unref_guard::operator=(packet_unref_guard &&other) {
  std::swap(m_packet, other.m_packet);
  return *this;
}

tl::optional<const codec &> find_decoder(codec_id id) {
  return optional_ref_from_ptr(avcodec_find_decoder(id));
}

tl::optional<const codec &> find_encoder(codec_id id) {
  return optional_ref_from_ptr(avcodec_find_encoder(id));
}

static tl::optional<const codec &> find_codec(codec_id id,
                                              codec_context_type type) {
  using enum codec_context_type;
  switch (type) {
  case decode:
    return find_decoder(id);
  case encode:
    return find_encoder(id);
  default:
    throw std::runtime_error("Invalid codec_context_type");
  }
}

codec_context::codec_context(codec_id id, codec_context_type type,
                             const codec_params *params)
    : codec_context{[&]() -> decltype(auto) {
                      auto codec = find_codec(id, type);
                      if (!codec.has_value()) {
                        throw std::runtime_error(fmt::format(
                            "Codec not supported: {}", static_cast<int>(id)));
                      }

                      return codec.value();
                    }(),
                    params} {}

codec_context::codec_context(const codec_params &params,
                             codec_context_type type)
    : codec_context{params.codec_id, type, &params} {}

codec_context::codec_context(const stream &stream, codec_context_type type)
    : codec_context{*stream.codecpar, type} {}

void codec_context_deleter::operator()(AVCodecContext *c) {
  avcodec_free_context(&c);
}

codec_context::codec_context(const codec &codec, const codec_params *params)
    : std::unique_ptr<AVCodecContext, codec_context_deleter>{call_alloc(
          throw_nested_runtime_error("Unable to allocate AVCodecContext"),
          avcodec_alloc_context3, &codec)} {
  if (params != nullptr) {
    call_and_handle_error(
        throw_nested_runtime_error(
            "Unable to set codec parameters in AVCodecContext"),
        avcodec_parameters_to_context, get(), params);
  }
}

void codec_context::init() {
  call_and_handle_error(
      throw_nested_runtime_error("Unable to open AVCodecContext"),
      avcodec_open2, get(), get()->codec, nullptr);
}

template <std::invocable<> NestedThrowCallback>
static auto to_send_recv_result(int ret, NestedThrowCallback &&callback) {
  if (ret == AVERROR(EAGAIN)) {
    return send_receive_result::eagain;
  }

  if (ret == AVERROR_EOF) {
    return send_receive_result::eof;
  }

  check_error(std::forward<NestedThrowCallback>(callback), ret);
  return send_receive_result::success;
}

send_receive_result codec_context::receive_frame(AVFrame *frame) {
  return to_send_recv_result(
      avcodec_receive_frame(get(), frame),
      throw_nested_runtime_error(
          "Unable to receive frame from AVCodecContext"));
}
send_receive_result codec_context::send_frame(const AVFrame *frame) {
  return to_send_recv_result(
      avcodec_send_frame(get(), frame),
      throw_nested_runtime_error("Unable to receive frame to AVCodecContext"));
}
send_receive_result codec_context::receive_packet(AVPacket *packet) {
  return to_send_recv_result(
      avcodec_receive_packet(get(), packet),
      throw_nested_runtime_error(
          "Unable to receive packet from AVCodecContext"));
}
send_receive_result codec_context::send_packet(const AVPacket *packet) {
  return to_send_recv_result(
      avcodec_send_packet(get(), packet),
      throw_nested_runtime_error("Unable to send packet to AVCodecContext"));
}

cppcoro::generator<hw_config> codec_context::hw_configs() {
  int i = 0;
  const AVCodecHWConfig *config;
  while ((config = avcodec_get_hw_config(this->get()->codec, i++)) != nullptr) {
    co_yield config;
  }
}

void codec_context::create_hwdevice(AVHWDeviceType type) {
  call_and_handle_error(
      throw_nested_runtime_error("Unable to create HWDevice of type {}",
                                 av_hwdevice_get_type_name(type)),
      av_hwdevice_ctx_create, &get()->hw_device_ctx, type, nullptr, nullptr, 0);
}

tl::optional<AVHWDeviceType> codec_context::create_hwdevice_auto() {
  std::unordered_set<hwdevice_type> supported_types;
  for (const auto type : hwdevice_types()) {
    supported_types.insert(type);
  }
  for (auto config : hw_configs()) {
    if (!supports_hwdevice_api(config) ||
        supported_types.find(config->device_type) == supported_types.end()) {
      continue;
    }

    try {
      create_hwdevice(config->device_type);
      return config->device_type;
    } catch (std::exception &ex) {
      log_exception(ex);
    }
  }

  return tl::nullopt;
}
void codec_context::set_format_callback(
    AVPixelFormat (*format_callback)(AVCodecContext *, const AVPixelFormat *)) {
  get()->get_format = format_callback;
}
} // namespace libved::ffmpeg
