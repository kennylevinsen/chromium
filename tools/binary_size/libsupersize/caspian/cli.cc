// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Command-line interface for checking the integrity of .size files.
// Usage: cli (path to .size file)

#include <stdlib.h>

#include <algorithm>
#include <fstream>
#include <iostream>

#include "tools/binary_size/libsupersize/caspian/diff.h"
#include "tools/binary_size/libsupersize/caspian/file_format.h"
#include "tools/binary_size/libsupersize/caspian/model.h"

void ParseSizeInfoFromFile(const char* filename, caspian::SizeInfo* info) {
  std::ifstream ifs(filename, std::ifstream::in);
  if (!ifs.good()) {
    std::cerr << "Unable to open file: " << filename << std::endl;
    exit(1);
  }
  std::string compressed((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
  caspian::ParseSizeInfo(compressed.data(), compressed.size(), info);
}

void Diff(const char* before_filename, const char* after_filename) {
  caspian::SizeInfo before;
  ParseSizeInfoFromFile(before_filename, &before);

  caspian::SizeInfo after;
  ParseSizeInfoFromFile(after_filename, &after);

  caspian::DeltaSizeInfo diff = Diff(&before, &after);

  float pss = 0.0f;
  float size = 0.0f;
  float padding = 0.0f;
  for (const auto& sym : diff.delta_symbols) {
    pss += sym.Pss();
    size += sym.Size();
    padding += sym.Padding();
  }
  std::cout << "Pss: " << pss << std::endl;
  std::cout << "Size: " << size << std::endl;
  std::cout << "Padding: " << padding << std::endl;
}

void Validate(const char* filename) {
  caspian::SizeInfo info;
  ParseSizeInfoFromFile(filename, &info);

  size_t max_aliases = 0;
  for (auto& s : info.raw_symbols) {
    if (s.aliases_) {
      max_aliases = std::max(max_aliases, s.aliases_->size());
      // What a wonderful O(n^2) loop
      for (auto* ss : *s.aliases_) {
        if (ss->aliases_ != s.aliases_) {
          std::cerr << "Not all symbols in alias group had same alias count"
                    << std::endl;
          exit(1);
        }
      }
    }
  }
  std::cout << "Largest number of aliases: " << max_aliases << std::endl;
}

void PrintUsageAndExit() {
  std::cerr << "Must have exactly one of:" << std::endl;
  std::cerr << "  validate, diff" << std::endl;
  std::cerr << "Usage:" << std::endl;
  std::cerr << "  cli validate <size file>" << std::endl;
  std::cerr << "  cli diff <before_file> <after_file>" << std::endl;
  exit(1);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    PrintUsageAndExit();
  }
  if (std::string_view(argv[1]) == "diff") {
    Diff(argv[2], argv[3]);
  } else if (std::string_view(argv[1]) == "validate") {
    Validate(argv[2]);
  } else {
    PrintUsageAndExit();
  }
  return 0;
}
