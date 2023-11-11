#pragma once

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace libved {
template <typename... Args>
inline auto throw_nested_runtime_error(fmt::format_string<Args...> fmt,
                                       Args &&...args) -> decltype(auto) {
  return [fmt, ... args = std::forward<Args>(args)]() mutable {
    if constexpr (sizeof...(args) == 0) {
      std::throw_with_nested(std::runtime_error{fmt.get().data()});
    } else {
      const auto message = fmt::format(fmt, std::forward<Args>(args)...);
      std::throw_with_nested(std::runtime_error{message});
    }
  };
}

inline auto no_throw_nested() {
  return [] { throw; };
}

static void log_exception(std::exception &ex, int rec_level = 0) {
  try {
    spdlog::warn("{}exception: {}", std::string(rec_level, ' '), ex.what());
    std::rethrow_if_nested(ex);
  } catch (std::exception &e) {
    log_exception(e, rec_level + 1);
  } catch (...) {
    spdlog::error("Unknown exception");
  }
}
} // namespace libved
