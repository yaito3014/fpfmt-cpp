#!/bin/sh
# SPDX-License-Identifier: MIT
# Part of fpfmt-cpp; see the license notice in include/yk/fpfmt.hpp.
#
# Differential test against the Go original: generates inputs, runs
# rsc.io/fpfmt (pinned in go.mod) on them, and checks that each given build
# of yk_fpfmt_go_differential prints identical results.
#
# usage: test/go_differential/run.sh EXECUTABLE...
#        (needs Go; the module download needs network access the first time)

set -eu

if [ "$#" -eq 0 ]; then
    echo "usage: $0 EXECUTABLE..." >&2
    exit 2
fi

here=$(cd "$(dirname "$0")" && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

ivy="$(go -C "$here" list -m -f '{{.Dir}}' rsc.io/fpfmt)/unopt/testdata/test.ivy"
go -C "$here" run . gen "$work/inputs.txt" "$ivy"
go -C "$here" run . run "$work/inputs.txt" > "$work/go.out"
lines=$(wc -l < "$work/go.out")

status=0
for exe in "$@"; do
    "$exe" "$work/inputs.txt" > "$work/cpp.out"
    if cmp -s "$work/go.out" "$work/cpp.out"; then
        echo "$exe: identical to Go ($lines lines)"
    else
        echo "$exe: differs from Go; first differences (< Go, > C++):"
        diff "$work/go.out" "$work/cpp.out" | head -n 40 || true
        status=1
    fi
done
exit "$status"
