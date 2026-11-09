// SPDX-License-Identifier: MIT AND BSD-3-Clause
// Part of fpfmt-cpp, derived from rsc.io/fpfmt; see the license notice in <yk/fpfmt.hpp>.

#ifndef YK_FPFMT_FORMAT_HPP
#define YK_FPFMT_FORMAT_HPP

#include <yk/fpfmt/detail/bits.hpp>
#include <yk/fpfmt/detail/config.hpp>
#include <yk/fpfmt/detail/pow10_table.hpp>
#include <yk/fpfmt/detail/unrounded.hpp>
#include <yk/fpfmt/detail/uscale.hpp>

#include <cassert>

namespace yk {

namespace fpfmt {

// decimal represents the value d * 10**p.
struct decimal {
  u64 d;
  int p;
};

namespace detail {

// skewed computes the skewed footprint of m * 2**e,
// which is floor(log10(3/4 * 2**e)) = floor(e*log10(2) - log10(4/3)).
constexpr int skewed(int e) noexcept { return (e * 631305 - 261663) >> 21; }

// trim_zeros removes trailing zeros from x * 10**p.
// If x ends in k zeros, trim_zeros returns x/10**k, p+k.
// It assumes that x ends in at most 16 zeros.
YK_FPFMT_CONSTEXPR14 decimal trim_zeros(u64 x, int p) noexcept
{
  u64 const max_u64 = ~u64(0);
  u64 const inv5p8 = 0xc767074b22e90e21;  // inverse of 5**8
  u64 const inv5p4 = 0xd288ce703afb7e91;  // inverse of 5**4
  u64 const inv5p2 = 0x8f5c28f5c28f5c29;  // inverse of 5**2
  u64 const inv5 = 0xcccccccccccccccd;    // inverse of 5

  // Cut 1 zero, or else return.
  u64 d = rotr64(x * inv5, 1);
  if (d <= max_u64 / 10) {
    x = d;
    p += 1;
  } else {
    return decimal{x, p};
  }

  // Cut 8 zeros, then 4, then 2, then 1.
  d = rotr64(x * inv5p8, 8);
  if (d <= max_u64 / 100000000) {
    x = d;
    p += 8;
  }
  d = rotr64(x * inv5p4, 4);
  if (d <= max_u64 / 10000) {
    x = d;
    p += 4;
  }
  d = rotr64(x * inv5p2, 2);
  if (d <= max_u64 / 100) {
    x = d;
    p += 2;
  }
  d = rotr64(x * inv5, 1);
  if (d <= max_u64 / 10) {
    x = d;
    p += 1;
  }
  return decimal{x, p};
}

// i2a is the formatting of 00..99 concatenated,
// a lookup table for formatting [0, 99].
constexpr char const* i2a() noexcept
{
  return "00010203040506070809"
         "10111213141516171819"
         "20212223242526272829"
         "30313233343536373839"
         "40414243444546474849"
         "50515253545556575859"
         "60616263646566676869"
         "70717273747576777879"
         "80818283848586878889"
         "90919293949596979899";
}

// format_base10 formats the decimal representation of u into a[0, nd).
// The caller is responsible for ensuring that nd is big enough to hold u.
// If nd is too big, leading zeros will be filled in as needed.
YK_FPFMT_CONSTEXPR14 void format_base10(char* a, int nd, u64 u) noexcept
{
  char const* const t = i2a();
  while (nd >= 8) {
    // Format last 8 digits (4 pairs).
    std::uint32_t const x3210 = std::uint32_t(u % 100000000);
    u /= 100000000;
    std::uint32_t const x32 = x3210 / 10000, x10 = x3210 % 10000;
    std::uint32_t const x1 = (x10 / 100) * 2, x0 = (x10 % 100) * 2;
    std::uint32_t const x3 = (x32 / 100) * 2, x2 = (x32 % 100) * 2;
    a[nd - 1] = t[x0 + 1], a[nd - 2] = t[x0];
    a[nd - 3] = t[x1 + 1], a[nd - 4] = t[x1];
    a[nd - 5] = t[x2 + 1], a[nd - 6] = t[x2];
    a[nd - 7] = t[x3 + 1], a[nd - 8] = t[x3];
    nd -= 8;
  }

  std::uint32_t x = std::uint32_t(u);
  if (nd >= 4) {
    // Format last 4 digits (2 pairs).
    std::uint32_t const x10 = x % 10000;
    x /= 10000;
    std::uint32_t const x1 = (x10 / 100) * 2, x0 = (x10 % 100) * 2;
    a[nd - 1] = t[x0 + 1], a[nd - 2] = t[x0];
    a[nd - 3] = t[x1 + 1], a[nd - 4] = t[x1];
    nd -= 4;
  }
  if (nd >= 2) {
    // Format last 2 digits.
    std::uint32_t const x0 = (x % 100) * 2;
    x /= 100;
    a[nd - 1] = t[x0 + 1], a[nd - 2] = t[x0];
    nd -= 2;
  }
  if (nd > 0) {
    // Format final digit.
    a[0] = char('0' + x);
  }
}

}  // namespace detail

// fixed_width returns the n-digit decimal form of f as d * 10**p.
// n can be at most 18. f must be finite and non-zero; its sign is ignored.
YK_FPFMT_CONSTEXPR_FP decimal fixed_width(f64 f, int n) noexcept
{
  assert(n <= 18 && "too many digits");
  detail::unpacked const up = detail::unpack64(f);
  int p = n - 1 - detail::log10pow2(up.e + 63);
  detail::unrounded const u = detail::uscale(up.m, detail::prescale(up.e, p, detail::log2pow10(p)));
  u64 d = u.round();
  if (d >= detail::uint64pow10(n)) {
    d = (u / 10).round();
    p = p - 1;
  }
  return decimal{d, -p};
}

// shortest computes the shortest formatting of f as d * 10**p,
// using as few digits as possible that will still round trip
// back to the original f64.
// f must be finite and non-zero; its sign is ignored.
// (This is Short in the Go original; "short" is a C++ keyword.)
YK_FPFMT_CONSTEXPR_FP decimal shortest(f64 f) noexcept
{
  int const min_exp = -1085;

  detail::unpacked const up = detail::unpack64(f);
  u64 const m = up.m;
  int const e = up.e;

  int p;
  u64 lo;
  int z = 11;  // extra zero bits at bottom of m; 11 for 53-bit m
  if (m == u64(1) << 63 && e > min_exp) {
    p = -detail::skewed(e + z);
    lo = m - (u64(1) << (z - 2));  // lo = m - 1/4 * 2**(e+z)
  } else {
    if (e < min_exp) z = 11 + (min_exp - e);
    p = -detail::log10pow2(e + z);
    lo = m - (u64(1) << (z - 1));  // lo = m - 1/2 * 2**(e+z)
  }
  u64 const hi = m + (u64(1) << (z - 1));  // hi = m + 1/2 * 2**(e+z)
  int const odd = int(m >> z) & 1;

  detail::scaler const pre = detail::prescale(e, p, detail::log2pow10(p));
  u64 const dmin = detail::uscale(lo, pre).nudge(+odd).ceil();
  u64 const dmax = detail::uscale(hi, pre).nudge(-odd).floor();

  u64 d = dmax / 10;
  if (d * 10 >= dmin) return detail::trim_zeros(d, -(p - 1));
  d = dmin;
  if (d < dmax) d = detail::uscale(m, pre).round();
  return decimal{d, -p};
}

// digits returns the number of decimal digits in d.
// Unlike the Go original, which returns 0 for d == 0, it returns 1 so that the result can be passed to format.
YK_FPFMT_CONSTEXPR_BITOPS int digits(u64 d) noexcept
{
  if (d == 0) return 1;
  int const nd = detail::log10pow2(detail::bit_len64(d));
  return nd + (d >= detail::uint64pow10(nd));
}

// format formats d * 10**p into s in exponential notation (e.g. "1.25e-03").
// The caller must pass nd set to the number of digits in d (see digits()),
// and s must have room for at least nd + 6 characters (nd + 7 if nd > 1).
// It returns the number of characters written to s. No terminating null is written.
// (This is Fmt in the Go original.)
YK_FPFMT_CONSTEXPR14 int format(char* s, u64 d, int p, int nd) noexcept
{
  char const* const t = detail::i2a();

  // Put digits into s, leaving room for decimal point.
  detail::format_base10(s + 1, nd, d);
  p += nd - 1;

  // Move first digit up and insert decimal point.
  s[0] = s[1];
  int n = nd;
  if (n > 1) {
    s[1] = '.';
    n++;
  }

  // Add 2- or 3-digit exponent.
  s[n] = 'e';
  if (p < 0) {
    s[n + 1] = '-';
    p = -p;
  } else {
    s[n + 1] = '+';
  }
  if (p < 100) {
    s[n + 2] = t[p * 2];
    s[n + 3] = t[p * 2 + 1];
    return n + 4;
  }
  s[n + 2] = char('0' + p / 100);
  s[n + 3] = t[(p % 100) * 2];
  s[n + 4] = t[(p % 100) * 2 + 1];
  return n + 5;
}

}  // namespace fpfmt

}  // namespace yk

#endif  // YK_FPFMT_FORMAT_HPP
