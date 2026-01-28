#include <catch2/catch.hpp>

#include <yk/fpfmt.hpp>

#include <iostream>

TEST_CASE("unrounded")
{
  // rounding
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(6);
    CHECK(u.floor().value == 6);
    CHECK(u.round_half_down().value == 6);
    CHECK(u.round().value == 6);
    CHECK(u.round_half_up().value == 6);
    CHECK(u.ceil().value == 6);
  }
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(6.25);
    CHECK(u.floor().value == 6);
    CHECK(u.round_half_down().value == 6);
    CHECK(u.round().value == 6);
    CHECK(u.round_half_up().value == 6);
    CHECK(u.ceil().value == 7);
  }
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(6.5);
    CHECK(u.floor().value == 6);
    CHECK(u.round_half_down().value == 6);
    CHECK(u.round().value == 6);
    CHECK(u.round_half_up().value == 7);
    CHECK(u.ceil().value == 7);
  }
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(6.75);
    CHECK(u.floor().value == 6);
    CHECK(u.round_half_down().value == 7);
    CHECK(u.round().value == 7);
    CHECK(u.round_half_up().value == 7);
    CHECK(u.ceil().value == 7);
  }

  // division
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(15.1);
    CHECK((u / 6).round().value == 3);
  }

  // nudging
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(15);
    CHECK(u.nudge(-1).floor().value == 14);
    CHECK(u.floor().value == 15);
    CHECK(u.ceil().value == 15);
    CHECK(u.nudge(+1).ceil().value == 16);
  }
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(15.1);
    CHECK(u.nudge(-1).floor().value == 15);
    CHECK(u.floor().value == 15);
    CHECK(u.ceil().value == 16);
    CHECK(u.nudge(+1).ceil().value == 16);
  }
  {
    yk::fpfmt::unrounded const u = yk::fpfmt::unround(15.9);
    CHECK(u.nudge(-1).floor().value == 15);
    CHECK(u.floor().value == 15);
    CHECK(u.ceil().value == 16);
    CHECK(u.nudge(+1).ceil().value == 16);
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
    CHECK(u.floor().value == 1);
  }

  // 1 * 2^(-63) * 10^1 = 10
  {
    int p = 1;
    auto c = prescale(-63, p, log2pow10(p));
    auto u = uscale(one, c);
    CHECK(u.floor().value == 10);
  }

  // 1 * 2^(-63) * 10^2 = 100
  {
    int p = 2;
    auto c = prescale(-63, p, log2pow10(p));
    auto u = uscale(one, c);
    CHECK(u.floor().value == 100);
  }

  // 1 * 2^(-63) * 10^3 = 1000
  {
    int p = 3;
    auto c = prescale(-63, p, log2pow10(p));
    auto u = uscale(one, c);
    CHECK(u.floor().value == 1000);
  }

  // Test with different mantissa: 2 * 10^2 = 200
  {
    u64 const two = u64(1) << 63;  // still left-justified
    int p = 2;
    auto c = prescale(-62, p, log2pow10(p));  // e = -62 means x * 2^(-62) = 2^63 * 2^(-62) = 2
    auto u = uscale(two, c);
    CHECK(u.floor().value == 200);
  }

  // Test larger value: 5 * 10^2 = 500
  {
    // 5 = 101 binary, left-justified: 0xA000000000000000 = 5 * 2^61
    u64 const five = u64(0xA) << 60;
    int p = 2;
    auto c = prescale(-61, p, log2pow10(p));  // e = -61 to get 5 * 2^61 * 2^(-61) = 5
    auto u = uscale(five, c);
    CHECK(u.floor().value == 500);
  }

  // Test: 3 * 10^3 = 3000
  {
    // 3 = 11 binary, left-justified: 0xC000000000000000 = 3 * 2^62
    u64 const three = u64(0xC) << 60;
    int p = 3;
    auto c = prescale(-62, p, log2pow10(p));
    auto u = uscale(three, c);
    CHECK(u.floor().value == 3000);
  }

  // Test: 7 * 10^1 = 70
  {
    // 7 = 111 binary, left-justified: 0xE000000000000000 = 7 * 2^61
    u64 const seven = u64(0xE) << 60;
    int p = 1;
    auto c = prescale(-61, p, log2pow10(p));
    auto u = uscale(seven, c);
    CHECK(u.floor().value == 70);
  }
}
