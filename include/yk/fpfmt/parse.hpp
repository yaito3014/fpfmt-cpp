// SPDX-License-Identifier: MIT AND BSD-3-Clause
// Part of fpfmt-cpp, derived from rsc.io/fpfmt; see the license notice in <yk/fpfmt.hpp>.

#ifndef YK_FPFMT_PARSE_HPP
#define YK_FPFMT_PARSE_HPP

#include <yk/fpfmt/detail/bits.hpp>
#include <yk/fpfmt/detail/config.hpp>
#include <yk/fpfmt/detail/pow10_table.hpp>
#include <yk/fpfmt/detail/unrounded.hpp>
#include <yk/fpfmt/detail/uscale.hpp>

#include <cassert>
#include <cstddef>
#include <limits>

namespace yk {

namespace fpfmt {

namespace detail {

constexpr bool is_digit(char c) noexcept { return static_cast<unsigned char>(c - '0') <= 9; }

}  // namespace detail

// parse rounds d * 10**p to the nearest f64.
// d can have at most 19 digits.
//
// Unlike the Go original, which requires p to be within the range of its power-of-ten table
// and assumes the result does not overflow, this function returns 0 for underflow and +Inf for overflow.
YK_FPFMT_CONSTEXPR_FP f64 parse(u64 d, int p) noexcept
{
  assert(d <= 10000000000000000000u && "too many digits");
  if (d == 0 || p < detail::pow10_min) return 0;  // d * 10**p < 1e20 * 1e-349 rounds to 0
  if (p > detail::pow10_max) return std::numeric_limits<f64>::infinity();

  int const b = detail::bit_len64(d);
  int const lp = detail::log2pow10(p);
  int e = 53 - b - lp;
  if (e > 1074) e = 1074;
  detail::unrounded u = detail::uscale(d << (64 - b), detail::prescale(e - (64 - b), p, lp));

  // This block is branch-free code for:
  //	if (u.round() >= 1<<53) {
  //		u = u.rsh(1);
  //		e = e - 1;
  //	}
  int const s = u.data >= detail::unmin(u64(1) << 53).data;
  u = detail::unrounded{(u.data >> s) | (u.data & 1)};
  e = e - s;

  if (1075 - e >= 0x7ff) return std::numeric_limits<f64>::infinity();
  return detail::pack64(u.round(), -e);
}

struct parse_result {
  f64 value;
  bool ok;
};

// parse_text parses the decimal string [first, last)
// and returns the nearest floating point value.
// It returns {0, false} if the input is malformed.
YK_FPFMT_CONSTEXPR_FP parse_result parse_text(char const* first, char const* last) noexcept
{
  parse_result const fail = {0, false};
  std::ptrdiff_t const len = last - first;

  // Read digits.
  std::ptrdiff_t const max_digits = 19;
  u64 d = 0;     // decimal value of digits
  int frac = 0;  // count of digits after decimal point
  std::ptrdiff_t i = 0;
  for (; i < len && detail::is_digit(first[i]); i++) d = d * 10 + u64(first[i] - '0');
  if (i > max_digits) return fail;  // too many digits
  if (i < len && first[i] == '.') {
    i++;
    for (; i < len && detail::is_digit(first[i]); i++) {
      d = d * 10 + u64(first[i] - '0');
      frac++;
    }
    if (i == 1 || i > max_digits + 1) return fail;  // no digits or too many digits
  }
  if (i == 0) return fail;  // no digits

  // Read exponent.
  int p = 0;
  if (i < len && (first[i] == 'e' || first[i] == 'E')) {
    i++;
    int sign = +1;
    if (i < len) {
      if (first[i] == '-') {
        sign = -1;
        i++;
      } else if (first[i] == '+') {
        i++;
      }
    }
    if (i >= len || len - i > 3) return fail;  // missing or too large exponent
    for (; i < len && detail::is_digit(first[i]); i++) p = p * 10 + (first[i] - '0');
    p *= sign;
  }
  if (i != len) return fail;  // junk on end
  return parse_result{parse(d, p - frac), true};
}

}  // namespace fpfmt

}  // namespace yk

#endif  // YK_FPFMT_PARSE_HPP
