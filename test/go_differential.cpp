// SPDX-License-Identifier: MIT
// Part of fpfmt-cpp; see the license notice in <yk/fpfmt.hpp>.

// The C++ side of the differential test against the Go original: for the
// inputs written by `go run . gen` (see go_differential/main.go), prints the
// results of fpfmt-cpp in the format `go run . run` prints those of
// rsc.io/fpfmt.
//
// usage: yk_fpfmt_go_differential INPUTS

#include <yk/fpfmt.hpp>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
  using namespace yk::fpfmt;

  if (argc != 2) {
    std::cerr << "usage: yk_fpfmt_go_differential INPUTS\n";
    return 2;
  }
  std::ifstream in(argv[1]);
  if (!in) {
    std::cerr << "cannot open " << argv[1] << '\n';
    return 1;
  }

  std::string line;
  char buf[64];
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    if (line[0] == 'F') {
      f64 const f = detail::f64_from_bits(std::stoull(line.substr(2)));
      decimal const r = shortest(f);
      int const n = format(buf, r.d, r.p, digits(r.d));
      std::printf("S %llu %d %.*s\n", static_cast<unsigned long long>(r.d), r.p, n, buf);
      for (int k = 1; k <= 18; k++) {
        decimal const x = fixed_width(f, k);
        std::printf("X%d %llu %d\n", k, static_cast<unsigned long long>(x.d), x.p);
      }
      if (parse(r.d, r.p) != f) std::printf("ROUNDTRIP FAIL\n");
    } else if (line[0] == 'P') {
      std::string::size_type const space = line.find(' ', 2);
      u64 const d = std::stoull(line.substr(2, space - 2));
      int const p = std::stoi(line.substr(space + 1));
      std::printf("P %llu\n", static_cast<unsigned long long>(detail::f64_to_bits(parse(d, p))));
    } else if (line[0] == 'T') {
      // The Go side writes the text with %q; the generated texts need no escapes, so dropping the quotes unquotes them.
      std::string const s = line.substr(3, line.size() - 4);
      parse_result const r = parse_text(s.data(), s.data() + s.size());
      std::printf("T %llu %s\n", static_cast<unsigned long long>(detail::f64_to_bits(r.value)), r.ok ? "true" : "false");
    }
  }
  return 0;
}
