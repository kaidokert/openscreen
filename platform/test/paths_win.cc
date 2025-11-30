// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "platform/test/paths.h"

namespace openscreen {

const std::string& GetTestDataPath() {
  static const std::string data_path = OPENSCREEN_TEST_DATA_DIR;
  return data_path;
}

}  // namespace openscreen
