#include "common.hpp"

extern "C" {
#include <libavutil/error.h>
}

inline std::string error_to_string(int code) {
  char message[AV_ERROR_MAX_STRING_SIZE + 1];
  av_make_error_string(message, sizeof(message), code);
  return message;
}

namespace libved::ffmpeg {
ffmpeg_error::ffmpeg_error(int code)
    : std::runtime_error{error_to_string(code)}, m_error_code{code} {}

int ffmpeg_error::error_code() const noexcept {
  return m_error_code;
}
} // namespace libved::ffmpeg
