#include <catch2/catch.hpp>

#include <yk/fpfmt.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

TEST_CASE("unrounded")
{
  // rounding
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(6);
    CHECK(u.floor() == 6);
    CHECK(u.round_half_down() == 6);
    CHECK(u.round() == 6);
    CHECK(u.round_half_up() == 6);
    CHECK(u.ceil() == 6);
  }
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(6.25);
    CHECK(u.floor() == 6);
    CHECK(u.round_half_down() == 6);
    CHECK(u.round() == 6);
    CHECK(u.round_half_up() == 6);
    CHECK(u.ceil() == 7);
  }
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(6.5);
    CHECK(u.floor() == 6);
    CHECK(u.round_half_down() == 6);
    CHECK(u.round() == 6);
    CHECK(u.round_half_up() == 7);
    CHECK(u.ceil() == 7);
  }
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(6.75);
    CHECK(u.floor() == 6);
    CHECK(u.round_half_down() == 7);
    CHECK(u.round() == 7);
    CHECK(u.round_half_up() == 7);
    CHECK(u.ceil() == 7);
  }

  // division
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(15.1);
    CHECK((u / 6).round() == 3);
  }

  // nudging
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(15);
    CHECK(u.nudge(-1).floor() == 14);
    CHECK(u.floor() == 15);
    CHECK(u.ceil() == 15);
    CHECK(u.nudge(+1).ceil() == 16);
  }
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(15.1);
    CHECK(u.nudge(-1).floor() == 15);
    CHECK(u.floor() == 15);
    CHECK(u.ceil() == 16);
    CHECK(u.nudge(+1).ceil() == 16);
  }
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(15.9);
    CHECK(u.nudge(-1).floor() == 15);
    CHECK(u.floor() == 15);
    CHECK(u.ceil() == 16);
    CHECK(u.nudge(+1).ceil() == 16);
  }
}

TEST_CASE("prescale")
{
  using yk::fpfmt::log2pow10;
  using yk::fpfmt::prescale;

  // prescale returns pm (128-bit power of 10) and shift s
  // pm values come from pow10table, s = -(e + lp + 3)

  // p = 0: 1e0 * 2**127, pm = {0x8000000000000000, 0x0000000000000000}
  {
    int p = 0;
    int lp = log2pow10(p);  // 0
    auto c = prescale(0, p, lp);
    CHECK(c.pm.high == 0x8000000000000000);
    CHECK(c.pm.low == 0x0000000000000000);
    CHECK(c.s == -(0 + lp + 3));  // -3
  }

  // p = 1: 1e1 * 2**124, pm = {0xa000000000000000, 0x0000000000000000}
  {
    int p = 1;
    int lp = log2pow10(p);  // 3
    auto c = prescale(0, p, lp);
    CHECK(c.pm.high == 0xa000000000000000);
    CHECK(c.pm.low == 0x0000000000000000);
    CHECK(c.s == -(0 + lp + 3));  // -6
  }

  // p = 2: 1e2 * 2**121, pm = {0xc800000000000000, 0x0000000000000000}
  {
    int p = 2;
    int lp = log2pow10(p);  // 6
    auto c = prescale(0, p, lp);
    CHECK(c.pm.high == 0xc800000000000000);
    CHECK(c.pm.low == 0x0000000000000000);
    CHECK(c.s == -(0 + lp + 3));  // -9
  }

  // p = -1: 1e-1 * 2**131, pm = {0xcccccccccccccccd, 0x3333333333333333}
  {
    int p = -1;
    int lp = log2pow10(p);  // -4
    auto c = prescale(0, p, lp);
    CHECK(c.pm.high == 0xcccccccccccccccd);
    CHECK(c.pm.low == 0x3333333333333333);
    CHECK(c.s == -(0 + lp + 3));  // 1
  }

  // p = -2: 1e-2 * 2**134, pm = {0xa3d70a3d70a3d70b, 0xc28f5c28f5c28f5c}
  {
    int p = -2;
    int lp = log2pow10(p);  // -7
    auto c = prescale(0, p, lp);
    CHECK(c.pm.high == 0xa3d70a3d70a3d70b);
    CHECK(c.pm.low == 0xc28f5c28f5c28f5c);
    CHECK(c.s == -(0 + lp + 3));  // 4
  }

  // Test with non-zero e: shifts s accordingly
  {
    int p = 0;
    int lp = log2pow10(p);
    auto c = prescale(10, p, lp);
    CHECK(c.pm.high == 0x8000000000000000);
    CHECK(c.s == -(10 + lp + 3));  // -13
  }
}

TEST_CASE("mul64")
{
  using yk::fpfmt::mul64;

  // Simple cases
  {
    auto r = mul64(0, 0);
    CHECK(r.high == 0);
    CHECK(r.low == 0);
  }
  {
    auto r = mul64(1, 1);
    CHECK(r.high == 0);
    CHECK(r.low == 1);
  }
  {
    auto r = mul64(2, 3);
    CHECK(r.high == 0);
    CHECK(r.low == 6);
  }

  // Results that fit in 64 bits
  {
    auto r = mul64(0xFFFFFFFF, 0xFFFFFFFF);
    CHECK(r.high == 0);
    CHECK(r.low == 0xFFFFFFFE00000001);
  }

  // Results that overflow into high part
  {
    auto r = mul64(0x100000000, 0x100000000);
    CHECK(r.high == 1);
    CHECK(r.low == 0);
  }
  {
    auto r = mul64(0x8000000000000000, 2);
    CHECK(r.high == 1);
    CHECK(r.low == 0);
  }

  // Maximum values
  {
    auto r = mul64(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
    CHECK(r.high == 0xFFFFFFFFFFFFFFFE);
    CHECK(r.low == 0x0000000000000001);
  }
  {
    auto r = mul64(0xFFFFFFFFFFFFFFFF, 1);
    CHECK(r.high == 0);
    CHECK(r.low == 0xFFFFFFFFFFFFFFFF);
  }
  {
    auto r = mul64(0xFFFFFFFFFFFFFFFF, 2);
    CHECK(r.high == 1);
    CHECK(r.low == 0xFFFFFFFFFFFFFFFE);
  }
}

TEST_CASE("uscale")
{
  using yk::fpfmt::log2pow10;
  using yk::fpfmt::prescale;
  using yk::fpfmt::u64;
  using yk::fpfmt::uscale;

  // uscale computes unround(x * 2^e * 10^p) where x is left-justified
  // x = 0x8000000000000000 represents 2^63, so with e = -63 we get x * 2^e = 1

  u64 const one = u64(1) << 63;  // left-justified 1

  // 1 * 2^(-63) * 10^0 = 1
  {
    int p = 0;
    auto c = prescale(-63, p, log2pow10(p));
    auto u = uscale(one, c);
    CHECK(u.floor() == 1);
  }

  // 1 * 2^(-63) * 10^1 = 10
  {
    int p = 1;
    auto c = prescale(-63, p, log2pow10(p));
    auto u = uscale(one, c);
    CHECK(u.floor() == 10);
  }

  // 1 * 2^(-63) * 10^2 = 100
  {
    int p = 2;
    auto c = prescale(-63, p, log2pow10(p));
    auto u = uscale(one, c);
    CHECK(u.floor() == 100);
  }

  // 1 * 2^(-63) * 10^3 = 1000
  {
    int p = 3;
    auto c = prescale(-63, p, log2pow10(p));
    auto u = uscale(one, c);
    CHECK(u.floor() == 1000);
  }

  // Test with different mantissa: 2 * 10^2 = 200
  {
    u64 const two = u64(1) << 63;  // still left-justified
    int p = 2;
    auto c = prescale(-62, p, log2pow10(p));  // e = -62 means x * 2^(-62) = 2^63 * 2^(-62) = 2
    auto u = uscale(two, c);
    CHECK(u.floor() == 200);
  }

  // Test larger value: 5 * 10^2 = 500
  {
    // 5 = 101 binary, left-justified: 0xA000000000000000 = 5 * 2^61
    u64 const five = u64(0xA) << 60;
    int p = 2;
    auto c = prescale(-61, p, log2pow10(p));  // e = -61 to get 5 * 2^61 * 2^(-61) = 5
    auto u = uscale(five, c);
    CHECK(u.floor() == 500);
  }

  // Test: 3 * 10^3 = 3000
  {
    // 3 = 11 binary, left-justified: 0xC000000000000000 = 3 * 2^62
    u64 const three = u64(0xC) << 60;
    int p = 3;
    auto c = prescale(-62, p, log2pow10(p));
    auto u = uscale(three, c);
    CHECK(u.floor() == 3000);
  }

  // Test: 7 * 10^1 = 70
  {
    // 7 = 111 binary, left-justified: 0xE000000000000000 = 7 * 2^61
    u64 const seven = u64(0xE) << 60;
    int p = 1;
    auto c = prescale(-61, p, log2pow10(p));
    auto u = uscale(seven, c);
    CHECK(u.floor() == 70);
  }
}

TEST_CASE("uscale low-bits mask")
{
  using namespace yk::fpfmt;

  // Regression test: Go's `hi&(1<<(c.s&63)-1)` is `hi & ((1 << s) - 1)` (Go's << binds tighter than -).
  // Testing only bit s-1 instead made this case round 2.58e-26 down to 2e-26.
  f64 const f = f64_from_bits(0x3aa0000000000000);  // 2**-85 = 2.5849394142282115e-26
  decimal const r = fixed_width(f, 1);
  CHECK(r.d == 3);
  CHECK(r.p == -26);
}

TEST_CASE("unpack64 / pack64")
{
  using namespace yk::fpfmt;

  {
    unpacked const u = unpack64(1.0);
    CHECK(u.m == u64(1) << 63);
    CHECK(u.e == -63);
    CHECK(pack64(u.m >> 11, u.e + 11) == 1.0);
  }
  {
    unpacked const u = unpack64(-1.5);  // sign is ignored
    CHECK(u.m == u64(3) << 62);
    CHECK(u.e == -63);
  }
  {
    // smallest subnormal: 2**-1074
    unpacked const u = unpack64(f64_from_bits(1));
    CHECK(u.m == u64(1) << 63);
    CHECK(u.e == -1074 - 63);
    CHECK(pack64(1, -1074) == f64_from_bits(1));
  }
  {
    unpacked const u = unpack64(f64_from_bits(0x7fefffffffffffff));  // DBL_MAX
    CHECK(u.m == ~u64(0) << 11);
    CHECK(u.e == 1023 - 63);
  }
}

TEST_CASE("unrounded rsh")
{
  using yk::fpfmt::unround;
  CHECK(unround(12).rsh(1).data == unround(6).data);
  CHECK(unround(13).rsh(1).data == unround(6.5).data);
  CHECK(unround(13).rsh(2).data == unround(3.25).data);  // 3.25 -> 3.0+
  CHECK(unround(12.5).rsh(1).data == unround(6.25).data);
}

TEST_CASE("digits")
{
  using yk::fpfmt::digits;
  using yk::fpfmt::u64;
  CHECK(digits(1) == 1);
  CHECK(digits(9) == 1);
  CHECK(digits(10) == 2);
  CHECK(digits(99) == 2);
  CHECK(digits(100) == 3);
  CHECK(digits(12345678901234567890u) == 20);
  for (int n = 1; n <= 19; n++) {
    u64 const p = yk::fpfmt::uint64pow10(n);
    CHECK(digits(p - 1) == n);
    CHECK(digits(p) == n + 1);
  }
}

namespace {

std::string fmt(yk::fpfmt::decimal r)
{
  char buf[32];
  int const n = yk::fpfmt::format(buf, r.d, r.p, yk::fpfmt::digits(r.d));
  return std::string(buf, n);
}

}  // namespace

TEST_CASE("format")
{
  using yk::fpfmt::decimal;
  CHECK(fmt(decimal{1, 0}) == "1e+00");
  CHECK(fmt(decimal{125, -5}) == "1.25e-03");
  CHECK(fmt(decimal{5, -324}) == "5e-324");
  CHECK(fmt(decimal{17976931348623157u, 292}) == "1.7976931348623157e+308");
  CHECK(fmt(decimal{1234567890123456789u, 0}) == "1.234567890123456789e+18");
  CHECK(fmt(decimal{12345678901234567890u, -10}) == "1.2345678901234567890e+09");
}

TEST_CASE("shortest")
{
  using namespace yk::fpfmt;

  CHECK(fmt(shortest(1.0)) == "1e+00");
  CHECK(fmt(shortest(0.1)) == "1e-01");
  CHECK(fmt(shortest(0.3)) == "3e-01");
  CHECK(fmt(shortest(0.1 + 0.2)) == "3.0000000000000004e-01");
  CHECK(fmt(shortest(123.456)) == "1.23456e+02");
  CHECK(fmt(shortest(1e23)) == "1e+23");
  CHECK(fmt(shortest(f64_from_bits(1))) == "5e-324");
  CHECK(fmt(shortest(f64_from_bits(0x000fffffffffffff))) == "2.225073858507201e-308");
  CHECK(fmt(shortest(f64_from_bits(0x0010000000000000))) == "2.2250738585072014e-308");
  CHECK(fmt(shortest(f64_from_bits(0x7fefffffffffffff))) == "1.7976931348623157e+308");
  CHECK(fmt(shortest(-2.5)) == "2.5e+00");  // sign is ignored

  decimal const r = shortest(1000.0);
  CHECK(r.d == 1);
  CHECK(r.p == 3);
}

TEST_CASE("fixed_width")
{
  using namespace yk::fpfmt;

  {
    decimal const r = fixed_width(std::ldexp(0x118352262653f8, 19 - 52), 7);  // 0x1.18352262653f8p+19
    CHECK(r.d == 5738651);
    CHECK(r.p == -1);
  }
  CHECK(fmt(fixed_width(1.0, 1)) == "1e+00");
  CHECK(fmt(fixed_width(1.0, 3)) == "1.00e+00");
  CHECK(fmt(fixed_width(0.1, 17)) == "1.0000000000000001e-01");
  CHECK(fmt(fixed_width(0.1, 18)) == "1.00000000000000006e-01");
  CHECK(fmt(fixed_width(9.5, 1)) == "1e+01");    // round half to even, carries into a new digit
  CHECK(fmt(fixed_width(8.5, 1)) == "8e+00");    // round half to even
  CHECK(fmt(fixed_width(1234.5, 4)) == "1.234e+03");
  CHECK(fmt(fixed_width(99.99, 2)) == "1.0e+02");
}

TEST_CASE("parse")
{
  using namespace yk::fpfmt;

  CHECK(parse(0, 0) == 0.0);
  CHECK(parse(1, 0) == 1.0);
  CHECK(parse(1, -1) == 0.1);
  CHECK(parse(3, -1) == 0.3);
  CHECK(parse(123456, -3) == 123.456);
  CHECK(parse(17976931348623157u, 292) == f64_from_bits(0x7fefffffffffffff));
  CHECK(parse(5, -324) == f64_from_bits(1));
  CHECK(parse(2, -324) == 0.0);  // below half of the smallest subnormal
  CHECK(parse(3, -324) == f64_from_bits(1));
  CHECK(parse(22250738585072014u, -324) == f64_from_bits(0x0010000000000000));
  CHECK(parse(9007199254740993u, 0) == 9007199254740992.0);  // 2**53 + 1: tie rounds to even
  CHECK(parse(9007199254740995u, 0) == 9007199254740996.0);  // 2**53 + 3: tie rounds to even
  CHECK(parse(10000000000000000000u, 0) == 1e19);

  // Out-of-range exponents (the Go original requires p in [-348, 347]).
  CHECK(parse(1, -400) == 0.0);
  CHECK(parse(10000000000000000000u, -349) == 0.0);
  CHECK(parse(1, 400) == HUGE_VAL);
  CHECK(parse(18, 307) == HUGE_VAL);
  CHECK(parse(17976931348623159u, 292) == HUGE_VAL);
}

TEST_CASE("parse_text")
{
  using namespace yk::fpfmt;

  struct ok_case {
    char const* s;
    f64 want;
  };
  ok_case const oks[] = {
      {"1", 1.0},
      {"1.", 1.0},
      {".5", 0.5},
      {"1.5", 1.5},
      {"1e-5", 1e-5},
      {"1E10", 1e10},
      {"12.34e+56", 12.34e+56},
      {"0.000", 0.0},
      {"1234567890123456789", 1234567890123456789.0},
      {"1.234567890123456789", 1.234567890123456789},
      {"123456789012345678.9", 123456789012345678.9},
      {"1e-0", 1.0},
      {"9e307", 9e307},
      {"2.2250738585072014e-308", 2.2250738585072014e-308},
  };
  for (ok_case const& c : oks) {
    CAPTURE(c.s);
    parse_result const r = parse_text(c.s, c.s + std::strlen(c.s));
    CHECK(r.ok);
    CHECK(r.value == c.want);
  }

  char const* const bads[] = {
      "",
      ".",
      "e5",
      "1e",
      "1e+",
      "1e1000",
      "abc",
      "1x",
      "-1",
      "+1",
      "12345678901234567890",
      "1.2345678901234567890",
      "1.5 ",
  };
  for (char const* s : bads) {
    CAPTURE(s);
    parse_result const r = parse_text(s, s + std::strlen(s));
    CHECK_FALSE(r.ok);
  }
}

namespace {

// splitmix64, for deterministic pseudo-random test inputs.
struct splitmix64 {
  std::uint64_t state;
  std::uint64_t operator()()
  {
    std::uint64_t z = (state += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
  }
};

// check compares fixed_width and shortest against the C library,
// whose printf and strtod are correctly rounded on glibc.
void check_against_libc(double f)
{
  using namespace yk::fpfmt;
  CAPTURE(f);
  char want[64];

  // shortest round trips through both our parse and strtod, and no shorter fixed width does.
  decimal const s = shortest(f);
  int const nd = digits(s.d);
  CHECK(parse(s.d, s.p) == f);
  std::string const str = fmt(s);
  CHECK(std::strtod(str.c_str(), nullptr) == f);
  if (nd > 1) {
    std::snprintf(want, sizeof want, "%.*e", nd - 2, f);
    CHECK(std::strtod(want, nullptr) != f);
  }

  // fixed_width matches printf's correctly rounded %e.
  for (int n : {1, 2, 6, 15, 16, 17, 18}) {
    CAPTURE(n);
    std::snprintf(want, sizeof want, "%.*e", n - 1, f);
    decimal const r = fixed_width(f, n);
    CHECK(fmt(r) == want);
    if (n >= 17) CHECK(parse(r.d, r.p) == f);
  }
}

}  // namespace

TEST_CASE("against libc")
{
  using namespace yk::fpfmt;

  for (int e = -1074; e <= 1023; e++) check_against_libc(std::ldexp(1.0, e));
  for (int p = -323; p <= 308; p++) check_against_libc(std::strtod(("1e" + std::to_string(p)).c_str(), nullptr));

  splitmix64 rng{12345};
  for (int i = 0; i < 20000; i++) {
    u64 b = rng() & ~(u64(1) << 63);
    if ((b >> 52) == 0x7ff || b == 0) continue;
    check_against_libc(f64_from_bits(b));
  }
  for (int i = 0; i < 2000; i++) {
    u64 b = rng() & ((u64(1) << 52) - 1);  // subnormal
    if (b == 0) continue;
    check_against_libc(f64_from_bits(b));
  }
}
