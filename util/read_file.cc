// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/read_file.h"

#include <algorithm>
#include <cstdio>

namespace openscreen {

FILE* OpenFile(std::string_view filename, const char* mode) {
#if defined(_WIN32)
  std::string win_filename(filename);
  std::replace(win_filename.begin(), win_filename.end(), '/', '\\');
  return fopen(win_filename.c_str(), mode);
#else
  return fopen(filename.data(), mode);
#endif
}

std::string ReadEntireFileToString(std::string_view filename) {
  FILE* file = OpenFile(filename, "rb");
  if (file == nullptr) {
    return {};
  }
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);  // NOLINT
  fseek(file, 0, SEEK_SET);
  std::string contents(file_size, 0);
  int bytes_read = 0;
  while (bytes_read < file_size) {
    size_t ret = fread(&contents[bytes_read], 1, file_size - bytes_read, file);
    if (ret == 0 && ferror(file)) {
      return {};
    } else {
      bytes_read += ret;
    }
  }
  fclose(file);

  return contents;
}

}  // namespace openscreen
