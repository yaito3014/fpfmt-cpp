// SPDX-License-Identifier: MIT AND BSD-3-Clause
// Part of fpfmt-cpp, derived from rsc.io/fpfmt; see the license notice in <yk/fpfmt.hpp>.

#ifndef YK_FPFMT_DETAIL_BITS_HPP
#define YK_FPFMT_DETAIL_BITS_HPP

#include <yk/fpfmt/detail/config.hpp>

#include <cstring>

namespace yk {

namespace fpfmt {

namespace detail {

// bit_len64 returns the minimum number of bits required to represent x (0 for x == 0).
YK_FPFMT_CONSTEXPR_BITOPS int bit_len64(u64 x) noexcept
{
#if defined(__cpp_lib_bitops)
  return 64 - std::countl_zero(x);
#elif defined(__GNUC__) || defined(__clang__)
  return x == 0 ? 0 : 64 - __builtin_clzll(x);
#else
  int n = 0;
  if (x >> 32) { x >>= 32; n += 32; }
  if (x >> 16) { x >>= 16; n += 16; }
  if (x >> 8) { x >>= 8; n += 8; }
  if (x >> 4) { x >>= 4; n += 4; }
  if (x >> 2) { x >>= 2; n += 2; }
  if (x >> 1) { x >>= 1; n += 1; }
  return n + int(x);
#endif
}

constexpr u64 rotr64(u64 x, int k) noexcept { return (x >> k) | (x << (64 - k)); }

YK_FPFMT_CONSTEXPR_BIT_CAST u64 f64_to_bits(f64 f) noexcept
{
#if defined(__cpp_lib_bit_cast)
  return std::bit_cast<u64>(f);
#else
  u64 b;
  std::memcpy(&b, &f, sizeof b);
  return b;
#endif
}

YK_FPFMT_CONSTEXPR_BIT_CAST f64 f64_from_bits(u64 b) noexcept
{
#if defined(__cpp_lib_bit_cast)
  return std::bit_cast<f64>(b);
#else
  f64 f;
  std::memcpy(&f, &b, sizeof f);
  return f;
#endif
}

struct unpacked {
  u64 m;
  int e;
};

// unpack64 returns m, e such that f = m * 2**e, with m left-justified (high bit set).
// The caller is expected to have handled 0, NaN, and +-Inf already.
// The sign of f is ignored.
YK_FPFMT_CONSTEXPR_FP unpacked unpack64(f64 f) noexcept
{
  int const shift = 64 - 53;
  int const min_exp = -(1074 + shift);
  u64 const b = f64_to_bits(f);
  u64 m = (u64(1) << 63) | ((b & ((u64(1) << 52) - 1)) << shift);
  int e = int((b >> 52) & ((u64(1) << shift) - 1));
  if (e == 0) {
    m &= ~(u64(1) << 63);
    e = min_exp;
    int const s = 64 - bit_len64(m);
    return unpacked{m << s, e - s};
  }
  return unpacked{m, (e - 1) + min_exp};
}

// pack64 takes m, e and returns f = m * 2**e.
// It assumes the caller has provided a 53-bit mantissa m
// and an exponent that is in range for the mantissa.
YK_FPFMT_CONSTEXPR_BIT_CAST f64 pack64(u64 m, int e) noexcept
{
  if ((m & (u64(1) << 52)) == 0) return f64_from_bits(m);
  return f64_from_bits((m & ~(u64(1) << 52)) | (u64(1075 + e) << 52));
}

}  // namespace detail

}  // namespace fpfmt

}  // namespace yk

#endif  // YK_FPFMT_DETAIL_BITS_HPP
