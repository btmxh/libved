#include "avutil.hpp"
#include "ffmpeg/wrappers/common.hpp"
#include <libavutil/frame.h>

namespace libved::ffmpeg {
frame alloc_frame() {
  return call_alloc(throw_nested_runtime_error("Unable to allocate AVFrame"),
                    av_frame_alloc);
}

frame::frame(AVFrame *f) : std::unique_ptr<AVFrame, frame_deleter>{f} {}
void frame_deleter::operator()(AVFrame *f) { av_frame_free(&f); }
frame_unref_guard::frame_unref_guard(AVFrame *f) : m_frame{f} {}
frame_unref_guard::frame_unref_guard(const frame &f)
    : frame_unref_guard{f.get()} {}
frame_unref_guard::~frame_unref_guard() { av_frame_unref(m_frame); }
} // namespace libved::ffmpeg
