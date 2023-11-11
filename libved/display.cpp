#include "display.hpp"
#include "windowed_display.hpp"
#include <memory>

namespace libved {
std::unique_ptr<display> create_display(const display_params &params,
                                        bool render) {
  return std::make_unique<windowed::window>(params);
}

display::display() {}

} // namespace libved
