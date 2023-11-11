#pragma once

#include <map>
#include <queue>
#include "wrappers/avutil.hpp"
#include "wrappers/avformat.hpp"

namespace libved::ffmpeg {
class packet_queue {
public:
  packet_queue();

private:
  std::queue<packet> m_queue;
};
} // namespace libved::ffmpeg
