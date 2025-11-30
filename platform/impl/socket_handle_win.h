// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_SOCKET_HANDLE_WIN_H_
#define PLATFORM_IMPL_SOCKET_HANDLE_WIN_H_

#include <winsock2.h>

#include "platform/impl/socket_handle.h"

namespace openscreen {

struct SocketHandle {
  explicit SocketHandle(SOCKET descriptor);
  SOCKET handle;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_SOCKET_HANDLE_WIN_H_
