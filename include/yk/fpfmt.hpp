#include <cmath>
#include <cstdint>

namespace yk {

namespace fpfmt {

using u64 = std::uint64_t;
using f64 = double;

struct f64repr {
  u64 value;
};

struct unrounded {
  f64repr data;

  f64repr floor() const noexcept { return f64repr{data.value >> 2}; }
  f64repr round_half_down() const noexcept { return f64repr{(data.value + 1) >> 2}; }
  f64repr round() const noexcept { return f64repr{((data.value + 1 + ((data.value >> 2) & 1)) >> 2)}; }
  f64repr round_half_up() const noexcept { return f64repr{(data.value + 2) >> 2}; }
  f64repr ceil() const noexcept { return f64repr{(data.value + 3) >> 2}; }

  unrounded nudge(int delta) const noexcept { return unrounded{f64repr{data.value + delta}}; }

  friend unrounded operator/(unrounded u, u64 d) noexcept { return unrounded{f64repr{(u.data.value / d) | (u.data.value & 1) | (u.data.value % d != 0)}}; }
};

inline unrounded unround(f64 x) noexcept
{
  f64 const quad = 4 * x;
  f64 const floored = std::floor(quad);
  return unrounded{f64repr{u64(floored) | (floored != quad)}};
}

}  // namespace fpfmt

}  // namespace yk
