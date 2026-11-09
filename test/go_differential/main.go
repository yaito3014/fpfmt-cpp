// SPDX-License-Identifier: MIT
// Part of fpfmt-cpp; see the license notice in include/yk/fpfmt.hpp.

// Command go_differential is the Go side of the differential test between
// fpfmt-cpp and the Go original, rsc.io/fpfmt, pinned in go.mod.
//
//	go run . gen INPUTS TEST_IVY  writes test inputs to INPUTS
//	go run . run INPUTS           prints the results of rsc.io/fpfmt for INPUTS
//
// TEST_IVY is unopt/testdata/test.ivy of rsc.io/fpfmt, whose hard cases are
// added to the inputs. The inputs are deterministic. ../go_differential.cpp
// prints the results of fpfmt-cpp in the same format, so the two outputs must
// be identical; run.sh does all of this.
package main

import (
	"bufio"
	"fmt"
	"math"
	"math/rand/v2"
	"os"
	"strconv"
	"strings"

	"rsc.io/fpfmt"
)

func main() {
	switch {
	case len(os.Args) == 4 && os.Args[1] == "gen":
		gen(os.Args[2], os.Args[3])
	case len(os.Args) == 3 && os.Args[1] == "run":
		run(os.Args[2])
	default:
		fmt.Fprintln(os.Stderr, "usage: go_differential gen INPUTS TEST_IVY | run INPUTS")
		os.Exit(2)
	}
}

// gen writes three kinds of input lines:
//
//	F <bits>   a finite non-zero float64, for Short, Fmt, Digits and FixedWidth
//	P <d> <p>  a Parse input in the range where the Go original is defined
//	T <quoted> a ParseText input, valid or not
func gen(path, ivyPath string) {
	f := must(os.Create(path))
	w := bufio.NewWriter(f)
	r := rand.New(rand.NewPCG(1, 2))

	var fs []float64
	for e := -1074; e <= 1023; e++ {
		fs = append(fs, math.Ldexp(1, e))
	}
	for p := -323; p <= 308; p++ {
		fs = append(fs, math.Pow10(p))
	}
	ivy := string(must(os.ReadFile(ivyPath)))
	for _, line := range strings.Split(ivy, "\n") {
		if i := strings.Index(line, "ftoa "); i >= 0 {
			if f, err := strconv.ParseFloat(strings.Fields(line[i+5:])[0], 64); err == nil {
				fs = append(fs, f)
			}
		}
	}
	for i := 0; i < 300000; i++ {
		b := r.Uint64() &^ (1 << 63)
		if b>>52 == 0x7ff {
			continue
		}
		fs = append(fs, math.Float64frombits(b))
	}
	for i := 0; i < 50000; i++ { // subnormals
		fs = append(fs, math.Float64frombits(r.Uint64N(1<<52)))
	}
	for _, f := range fs {
		if f == 0 || math.IsInf(f, 0) {
			continue
		}
		for _, g := range []float64{math.Nextafter(f, 0), f, math.Nextafter(f, math.Inf(1))} {
			if g != 0 && !math.IsInf(g, 0) {
				fmt.Fprintf(w, "F %d\n", math.Float64bits(g))
			}
		}
	}

	// Parse panics or returns garbage outside its table range and on overflow,
	// which fpfmt-cpp handles differently on purpose, so those are left out.
	for i := 0; i < 300000; i++ {
		d := r.Uint64N(10000000000000000000 + 1)
		if i%3 == 0 {
			d = r.Uint64N(1 << r.IntN(64))
		}
		p := r.IntN(347+348+1) - 348
		f, _ := strconv.ParseFloat(fmt.Sprintf("%de%d", d, p), 64)
		if math.IsInf(f, 0) || d == 0 {
			continue
		}
		fmt.Fprintf(w, "P %d %d\n", d, p)
	}

	texts := []string{"", ".", "1", "1.", ".5", "1.5", "1e", "1e+", "1e-5", "1E10", "12.34e+56", "1e1000",
		"abc", "1x", "0.000", "1234567890123456789", "12345678901234567890", "1.234567890123456789",
		"1.2345678901234567890", "123456789012345678.9", "-1", "+1", "1e-0", "00000000000000000001", "9e307"}
	for _, t := range texts {
		fmt.Fprintf(w, "T %q\n", t)
	}
	for i := 0; i < 50000; i++ {
		nd := 1 + r.IntN(19)
		s := strconv.FormatUint(r.Uint64N(uint64(math.Pow10(nd))), 10)
		if k := r.IntN(len(s) + 1); k < len(s) && r.IntN(2) == 0 {
			s = s[:k] + "." + s[k:]
		}
		s += fmt.Sprintf("e%d", r.IntN(500)-250)
		fmt.Fprintf(w, "T %q\n", s)
	}

	must(0, w.Flush())
	must(0, f.Close())
}

// run prints, for each input line:
//
//	F: "S <d> <p> <Fmt output>", "X<n> <d> <p>" for n in 1..18, and
//	   "ROUNDTRIP FAIL" if Parse(Short(f)) != f
//	P: "P <bits of Parse(d, p)>"
//	T: "T <bits of the result> <ok>"
func run(path string) {
	f := must(os.Open(path))
	defer f.Close()
	sc := bufio.NewScanner(f)
	w := bufio.NewWriter(os.Stdout)
	buf := make([]byte, 64)
	for sc.Scan() {
		line := sc.Text()
		switch line[0] {
		case 'F':
			b := must(strconv.ParseUint(line[2:], 10, 64))
			f := math.Float64frombits(b)
			d, p := fpfmt.Short(f)
			fmt.Fprintf(w, "S %d %d %s\n", d, p, buf[:fpfmt.Fmt(buf, d, p, fpfmt.Digits(d))])
			for n := 1; n <= 18; n++ {
				d, p := fpfmt.FixedWidth(f, n)
				fmt.Fprintf(w, "X%d %d %d\n", n, d, p)
			}
			if fpfmt.Parse(d, p) != f {
				fmt.Fprintf(w, "ROUNDTRIP FAIL\n")
			}
		case 'P':
			fields := strings.Fields(line[2:])
			d := must(strconv.ParseUint(fields[0], 10, 64))
			p := must(strconv.Atoi(fields[1]))
			fmt.Fprintf(w, "P %d\n", math.Float64bits(fpfmt.Parse(d, p)))
		case 'T':
			s := must(strconv.Unquote(line[2:]))
			f, ok := fpfmt.ParseText([]byte(s))
			fmt.Fprintf(w, "T %d %v\n", math.Float64bits(f), ok)
		}
	}
	must(0, sc.Err())
	must(0, w.Flush())
}

func must[T any](x T, err error) T {
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	return x
}
