# SPDX-License-Identifier: MIT AND BSD-3-Clause
# Part of fpfmt-cpp, derived from pow10gen.go of rsc.io/fpfmt;
# see the license notice in include/yk/fpfmt.hpp.
"""Generates include/yk/fpfmt/detail/pow10_table.hpp from tools/pow10_table.hpp.in.

The 128-bit table is computed as pow10gen.go of rsc.io/fpfmt computes it: for
each p in [POW10_MIN, POW10_MAX], 10**p is scaled by a power of two into
[2**127, 2**128), and the result is stored as hi * 2**64 - lo, rounded up, so
that every entry has its high bit set. Each row is commented with the scaling,
as in the Go table.

usage: python3 tools/gen_pow10_table.py [--check]
       (--check writes nothing and fails if the header is not up to date)
"""

import os
import re
import sys
from fractions import Fraction

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
TEMPLATE = os.path.join(ROOT, "tools", "pow10_table.hpp.in")
OUTPUT = os.path.join(ROOT, "include", "yk", "fpfmt", "detail", "pow10_table.hpp")

POW10_MIN = -348
POW10_MAX = 347
INDENT = " " * 6
U64 = 1 << 64


def pow10_128(p: int) -> tuple[int, int, int]:
    """Returns (hi, lo, be) such that 10**p * 2**be rounded up is hi * 2**64 - lo."""
    r = Fraction(10) ** p
    be = 0
    while r < 1 << 128:
        r *= 2
        be += 1
    while r >= 1 << 128:
        r /= 2
        be -= 1
    hi, lo = divmod(r.numerator // r.denominator, U64)
    if r.denominator != 1:
        lo = (lo + 1) % U64
        if lo == 0:
            hi += 1
    if lo != 0:
        hi += 1
        lo = U64 - lo
    assert 1 << 63 <= hi < U64 and 0 <= lo < U64, p
    return hi, lo, be


def render() -> str:
    rows_128 = []
    for p in range(POW10_MIN, POW10_MAX + 1):
        hi, lo, be = pow10_128(p)
        rows_128.append(f"{INDENT}{{0x{hi:016x}, 0x{lo:016x}}},  // 1e{p} * 2**{be}")
    rows_u64 = [f"{INDENT}{10**x}u," for x in range(20)]
    values = {
        "POW10_MIN": str(POW10_MIN),
        "POW10_MAX": str(POW10_MAX),
        "POW10_U64_ROWS": "\n".join(rows_u64),
        "POW10_128_ROWS": "\n".join(rows_128),
    }
    with open(TEMPLATE) as f:
        template = f.read()
    placeholders = set(re.findall(r"@([A-Z0-9_]+)@", template))
    assert placeholders == set(values), (
        f"placeholders {sorted(placeholders)} do not match {sorted(values)}"
    )
    return re.sub(r"@([A-Z0-9_]+)@", lambda m: values[m.group(1)], template)


def main() -> int:
    args = sys.argv[1:]
    if args not in ([], ["--check"]):
        print(__doc__, file=sys.stderr)
        return 2
    text = render()
    if args == ["--check"]:
        with open(OUTPUT) as f:
            if f.read() != text:
                print(
                    f"{os.path.relpath(OUTPUT, ROOT)} is out of date; run tools/gen_pow10_table.py",
                    file=sys.stderr,
                )
                return 1
        return 0
    with open(OUTPUT, "w") as f:
        f.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
