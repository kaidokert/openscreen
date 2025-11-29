// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_PLATFORM_CLIENT_UTILS_H_
#define PLATFORM_IMPL_PLATFORM_CLIENT_UTILS_H_

#if defined(_WIN32)
#include "platform/impl/platform_client_win.h"
#else
#include "platform/impl/platform_client_posix.h"
#endif

namespace openscreen {

#if defined(_WIN32)
using PlatformClient = PlatformClientWin;
#else
using PlatformClient = PlatformClientPosix;
#endif

}  // namespace openscreen

#endif  // PLATFORM_IMPL_PLATFORM_CLIENT_UTILS_H_
