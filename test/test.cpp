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
