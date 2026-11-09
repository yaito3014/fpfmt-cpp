// SPDX-License-Identifier: MIT
// Part of fpfmt-cpp; see the license notice in <yk/fpfmt.hpp>.

#ifndef YK_FPFMT_DETAIL_CONFIG_HPP
#define YK_FPFMT_DETAIL_CONFIG_HPP

#include <cstdint>

#if defined(__has_include)
#if __has_include(<version>)
#include <version>
#endif
#endif

#if defined(__cpp_lib_bitops) || defined(__cpp_lib_bit_cast)
#include <bit>
#endif

#if defined(_MSC_VER) && !defined(__SIZEOF_INT128__) && ((defined(_M_X64) && !defined(_M_ARM64EC)) || defined(_M_ARM64))
#define YK_FPFMT_MUL64_MSVC_INTRINSICS
#include <intrin.h>
#endif

// Each YK_FPFMT_CONSTEXPR_* macro expands to constexpr when the features the function needs are available,
// and to inline otherwise.
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304L
#define YK_FPFMT_CONSTEXPR14 constexpr
#else
#define YK_FPFMT_CONSTEXPR14 inline
#endif

#if defined(__cpp_lib_bitops)
#define YK_FPFMT_CONSTEXPR_BITOPS constexpr
#else
#define YK_FPFMT_CONSTEXPR_BITOPS inline
#endif

#if defined(__cpp_lib_bit_cast)
#define YK_FPFMT_CONSTEXPR_BIT_CAST constexpr
#else
#define YK_FPFMT_CONSTEXPR_BIT_CAST inline
#endif

// The MSVC intrinsics are not constexpr, so constant evaluation must be detected to avoid them.
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304L && (!defined(YK_FPFMT_MUL64_MSVC_INTRINSICS) || defined(__cpp_lib_is_constant_evaluated))
#define YK_FPFMT_CONSTEXPR_MUL64 constexpr
#define YK_FPFMT_HAS_CONSTEXPR_MUL64
#else
#define YK_FPFMT_CONSTEXPR_MUL64 inline
#endif

// For functions that need all of the above (unpack64, fixed_width, shortest, parse, parse_text).
#if defined(__cpp_lib_bitops) && defined(__cpp_lib_bit_cast) && defined(YK_FPFMT_HAS_CONSTEXPR_MUL64)
#define YK_FPFMT_CONSTEXPR_FP constexpr
#else
#define YK_FPFMT_CONSTEXPR_FP inline
#endif

namespace yk {

namespace fpfmt {

using u64 = std::uint64_t;
using f64 = double;

namespace detail {

// __SIZEOF_INT128__ is defined by GCC, Clang (including clang-cl), and compilers based on them,
// only on targets where unsigned __int128 is usable.
// __extension__ silences -Wpedantic's "ISO C++ does not support '__int128'".
#if defined(__SIZEOF_INT128__)
__extension__ typedef unsigned __int128 u128;
#endif

}  // namespace detail

}  // namespace fpfmt

}  // namespace yk

#endif  // YK_FPFMT_DETAIL_CONFIG_HPP
