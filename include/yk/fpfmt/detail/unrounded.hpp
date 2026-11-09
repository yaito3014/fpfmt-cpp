// SPDX-License-Identifier: MIT AND BSD-3-Clause
// Part of fpfmt-cpp, derived from rsc.io/fpfmt; see the license notice in <yk/fpfmt.hpp>.

#ifndef YK_FPFMT_DETAIL_UNROUNDED_HPP
#define YK_FPFMT_DETAIL_UNROUNDED_HPP

#include <yk/fpfmt/detail/config.hpp>

namespace yk {

namespace fpfmt {

namespace detail {

struct unrounded {
  u64 data;

  constexpr u64 floor() const noexcept { return data >> 2; }
  constexpr u64 round_half_down() const noexcept { return (data + 1) >> 2; }
  constexpr u64 round() const noexcept { return ((data + 1 + ((data >> 2) & 1)) >> 2); }
  constexpr u64 round_half_up() const noexcept { return (data + 2) >> 2; }
  constexpr u64 ceil() const noexcept { return (data + 3) >> 2; }

  constexpr unrounded nudge(int delta) const noexcept { return unrounded{data + u64(delta)}; }

  // rsh returns u >> s, preserving the sticky bit. s must be in [0, 63].
  constexpr unrounded rsh(int s) const noexcept { return unrounded{(data >> s) | (data & 1) | ((data & ((u64(1) << s) - 1)) != 0)}; }

  friend constexpr unrounded operator/(unrounded u, u64 d) noexcept { return unrounded{(u.data / d) | (u.data & 1) | (u.data % d != 0)}; }
};

// unmin returns the minimum unrounded that rounds to x.
constexpr unrounded unmin(u64 x) noexcept { return unrounded{(x << 2) - 2}; }

// unround returns the unrounded form of x, which must be in [0, 2**62).
// For non-negative values, conversion to u64 truncates, which is floor; std::floor is not constexpr before C++23.
constexpr unrounded unround(f64 x) noexcept { return unrounded{u64(4 * x) | (f64(u64(4 * x)) != 4 * x)}; }

}  // namespace detail

}  // namespace fpfmt

}  // namespace yk

#endif  // YK_FPFMT_DETAIL_UNROUNDED_HPP
