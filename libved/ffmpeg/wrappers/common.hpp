#pragma once

#include <concepts>
#include <exception>
#include <fmt/core.h>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <errors.hpp>

namespace libved::ffmpeg {
class ffmpeg_error : public std::runtime_error {
public:
  ffmpeg_error(int code);

  [[nodiscard]] int error_code() const noexcept;

private:
  int m_error_code;
};

inline void throw_if_not_success(int error_code) {
  if (error_code != 0) {
    throw ffmpeg_error{error_code};
  }
}

template <std::invocable<> NestedThrowCallback>
inline void check_error(NestedThrowCallback &&callback, int error) {
  try {
    throw_if_not_success(error);
  } catch (...) {
    std::forward<NestedThrowCallback>(callback)();
  }
}

template <std::invocable<> NestedThrowCallback, typename... Args,
          std::invocable<Args &&...> Func>
inline void call_and_handle_error(NestedThrowCallback &&callback, Func &&f,
                                  Args &&...args) {
  int error = std::forward<Func>(f)(std::forward<Args>(args)...);
  check_error(std::forward<NestedThrowCallback>(callback), error);
}

template <std::invocable<> NestedThrowCallback, typename... Args,
          std::invocable<Args &&...> Func>
  requires std::equality_comparable_with<std::invoke_result_t<Func, Args...>,
                                         std::nullptr_t>
inline decltype(auto) call_alloc(NestedThrowCallback &&callback, Func &&f,
                                 Args &&...args) {
  auto result = std::forward<Func>(f)(std::forward<Args>(args)...);
  try {
    if (result == nullptr) {
      throw std::bad_alloc{};
    }
  } catch (...) {
    std::forward<NestedThrowCallback>(callback)();
  }

  return result;
}
} // namespace libved::ffmpeg
