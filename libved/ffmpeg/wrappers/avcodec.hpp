#pragma once

#include <concepts>
#include <cppcoro/generator.hpp>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
}

#include "avutil.hpp"
#include "common.hpp"
#include <memory>
#include <tl/optional.hpp>

namespace libved::ffmpeg {
using codec = AVCodec;
using codec_id = AVCodecID;
using codec_params = AVCodecParameters;
using stream = AVStream;
using hw_config = const AVCodecHWConfig *;
using hwdevice_type = AVHWDeviceType;

struct packet_deleter {
  void operator()(AVPacket *p);
};

class packet : public std::unique_ptr<AVPacket, packet_deleter> {
public:
  packet(AVPacket *packet = nullptr);
};

[[nodiscard]] packet alloc_packet();

class packet_unref_guard {
public:
  packet_unref_guard(AVPacket *p);
  packet_unref_guard(const packet &p);
  ~packet_unref_guard();

  packet_unref_guard(const packet_unref_guard &) = delete;
  auto operator=(const packet_unref_guard &) = delete;

  packet_unref_guard(packet_unref_guard &&);
  packet_unref_guard &operator=(packet_unref_guard &&);

private:
  AVPacket *m_packet;
};

[[nodiscard]] tl::optional<const codec &> find_decoder(codec_id id);
[[nodiscard]] tl::optional<const codec &> find_encoder(codec_id id);

struct codec_context_deleter {
  void operator()(AVCodecContext *c);
};

enum class send_receive_result {
  success = 0,
  eagain,
  eof,
};

enum class codec_context_type {
  decode,
  encode,
};

class codec_context
    : public std::unique_ptr<AVCodecContext, codec_context_deleter> {
public:
  codec_context(const codec &codec, const codec_params *params = nullptr);
  codec_context(codec_id codec_id,
                codec_context_type type = codec_context_type::decode,
                const codec_params *params = nullptr);
  codec_context(const codec_params &codec_params,
                codec_context_type type = codec_context_type::decode);
  codec_context(const stream &stream,
                codec_context_type type = codec_context_type::decode);

  void init();

  send_receive_result send_frame(const AVFrame *frame);
  send_receive_result receive_frame(AVFrame *frame);
  send_receive_result send_packet(const AVPacket *packet);
  send_receive_result receive_packet(AVPacket *packet);

  decltype(auto) receive_frame(const frame &frm) {
    return receive_frame(frm.get());
  }

  decltype(auto) send_frame(const frame &frm) { return send_frame(frm.get()); }

  decltype(auto) send_packet(const packet &pkt) {
    return send_packet(pkt.get());
  }

  decltype(auto) receive_packet(const packet &pkt) {
    return receive_packet(pkt.get());
  }

  cppcoro::generator<hw_config> hw_configs();
  void create_hwdevice(AVHWDeviceType type);
  tl::optional<AVHWDeviceType> create_hwdevice_auto();

  void set_format_callback(
      AVPixelFormat (*format_callback)(AVCodecContext *, const AVPixelFormat *));
};

inline cppcoro::generator<hwdevice_type> hwdevice_types() {
  hwdevice_type type = AV_HWDEVICE_TYPE_NONE;
  while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
    co_yield type;
  }
}

inline bool supports_hwdevice_api(hw_config conf) {
  return conf->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX;
}
} // namespace libved::ffmpeg
