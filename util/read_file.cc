// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/read_file.h"

#include <errno.h>  // For errno
#include <stdio.h>

#include <algorithm>  // For std::replace
#if defined(_WIN32)
// clang-format off
#include <windows.h>  // For MAX_PATH

#include <shlwapi.h>  // For PathFileExistsA
#include <stdlib.h>   // For _fullpath
// clang-format on
#endif

namespace openscreen {

std::string ReadEntireFileToString(std::string_view filename) {
#if defined(_WIN32)
  std::string win_filename(filename);
  std::replace(win_filename.begin(), win_filename.end(), '/', '\\');
  FILE* file = fopen(win_filename.c_str(), "rb");
#else
  FILE* file = fopen(filename.data(), "rb");
#endif
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
