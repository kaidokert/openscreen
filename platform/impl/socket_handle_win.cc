// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/socket_handle_win.h"

namespace openscreen {

SocketHandle::SocketHandle(SOCKET descriptor) : handle(descriptor) {}

size_t SocketHandleHash::operator()(const SocketHandle& handle) const {
  return static_cast<size_t>(handle.handle);
}

bool operator==(const SocketHandle& lhs, const SocketHandle& rhs) {
  return lhs.handle == rhs.handle;
}

}  // namespace openscreen
