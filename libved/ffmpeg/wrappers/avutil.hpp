#pragma once

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
}

#include <memory>

namespace libved::ffmpeg {
struct frame_deleter {
  void operator()(AVFrame *f);
};
class frame : public std::unique_ptr<AVFrame, frame_deleter> {
public:
  frame(AVFrame* f = nullptr);
};

class frame_unref_guard {
public:
  frame_unref_guard(AVFrame* f);
  frame_unref_guard(const frame& f);
  ~frame_unref_guard();
private:
  AVFrame* m_frame;
};

[[nodiscard]] frame alloc_frame();

} // namespace libved::ffmpeg
