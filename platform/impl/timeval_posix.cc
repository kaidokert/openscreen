// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/timeval_posix.h"

#include <chrono>

#include "util/chrono_helpers.h"

namespace openscreen {

struct timeval ToTimeval(const Clock::duration& timeout) {
  struct timeval tv {};
  const auto whole_seconds = to_seconds(timeout);
  tv.tv_sec = whole_seconds.count();
  tv.tv_usec = to_microseconds(timeout - whole_seconds).count();

  return tv;
}

struct timespec ToTimespec(const Clock::duration& timeout) {
  struct timespec ts {};
  const auto whole_seconds = to_seconds(timeout);
  ts.tv_sec = whole_seconds.count();
  ts.tv_nsec = to_nanoseconds(timeout - whole_seconds).count();

  return ts;
}
}  // namespace openscreen
