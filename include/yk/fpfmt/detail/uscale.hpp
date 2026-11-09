// SPDX-License-Identifier: MIT AND BSD-3-Clause
// Part of fpfmt-cpp, derived from rsc.io/fpfmt; see the license notice in <yk/fpfmt.hpp>.

#ifndef YK_FPFMT_DETAIL_USCALE_HPP
#define YK_FPFMT_DETAIL_USCALE_HPP

#include <yk/fpfmt/detail/config.hpp>
#include <yk/fpfmt/detail/pow10_table.hpp>
#include <yk/fpfmt/detail/unrounded.hpp>

#include <type_traits>

namespace yk {

namespace fpfmt {

namespace detail {

// log10pow2(x) returns floor(log10(2**x)) = floor(x * log10(2)).
constexpr int log10pow2(int x) noexcept { return (x * 78913) >> 18; }

// log2pow10(x) returns floor(log2(10**x)) = floor(x * log2(10)).
constexpr int log2pow10(int x) noexcept { return (x * 108853) >> 15; }

struct scaler {
  u64x2 pm;
  int s;
};

// prescale returns the scaling constants for e, p.
// lp must be log2pow10(p). p must be in [pow10_min, pow10_max].
constexpr scaler prescale(int e, int p, int lp) noexcept { return scaler{tables<>::pow10_128[p - pow10_min], -(e + lp + 3)}; }

YK_FPFMT_CONSTEXPR14 u64x2 mul64_portable(u64 x, u64 y) noexcept
{
  u64 const x_lo = x & 0xFFFFFFFF;
  u64 const x_hi = x >> 32;
  u64 const y_lo = y & 0xFFFFFFFF;
  u64 const y_hi = y >> 32;

  u64 const lo_lo = x_lo * y_lo;
  u64 const hi_lo = x_hi * y_lo;
  u64 const lo_hi = x_lo * y_hi;
  u64 const hi_hi = x_hi * y_hi;

  u64 const mid = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
  u64 const high = hi_hi + (hi_lo >> 32) + (mid >> 32);
  u64 const low = (mid << 32) | (lo_lo & 0xFFFFFFFF);

  return u64x2{high, low};
}

YK_FPFMT_CONSTEXPR_MUL64 u64x2 mul64(u64 x, u64 y) noexcept
{
#if defined(__SIZEOF_INT128__)
  u128 const r = u128(x) * y;
  return u64x2{u64(r >> 64), u64(r)};
#elif defined(YK_FPFMT_MUL64_MSVC_INTRINSICS)
#if defined(__cpp_lib_is_constant_evaluated)
  if (std::is_constant_evaluated()) return mul64_portable(x, y);
#endif
#if defined(_M_ARM64)
  return u64x2{__umulh(x, y), x * y};
#else
  u64 high;
  u64 const low = _umul128(x, y, &high);
  return u64x2{high, low};
#endif
#else
  return mul64_portable(x, y);
#endif
}

// uscale returns unround(x * 2**e * 10**p).
// The caller should pass c = prescale(e, p, log2pow10(p))
// and should have left-justified x so its high bit is set.
YK_FPFMT_CONSTEXPR_MUL64 unrounded uscale(u64 x, scaler c) noexcept
{
  u64x2 hm = mul64(x, c.pm.high);
  bool sticky = true;
  if ((hm.high & ((u64(1) << (c.s & 63)) - 1)) == 0) {
    u64x2 m2 = mul64(x, c.pm.low);
    sticky = hm.low - m2.high > 1;
    hm.high -= hm.low < m2.high;
  }
  // c.s can exceed 63 when Parse produces a subnormal result; the shifted value is then 0.
  return unrounded{(c.s < 64 ? hm.high >> c.s : 0) | u64(sticky)};
}

}  // namespace detail

}  // namespace fpfmt

}  // namespace yk

#endif  // YK_FPFMT_DETAIL_USCALE_HPP
