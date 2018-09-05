#ifndef MOLPHENE_CLICK_STATE_HPP
#define MOLPHENE_CLICK_STATE_HPP

#include <molphene/stdafx.hpp>

namespace molphene {

class ClickState {
public:
  bool is_down;
  size_t last_x;
  size_t last_y;

  ClickState() noexcept = default;

  ClickState(bool is_down, size_t last_x, size_t last_y) noexcept;
};

} // namespace molphene

#endif
