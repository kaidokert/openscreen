// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_SOCKET_HANDLE_WAITER_WIN_H_
#define PLATFORM_IMPL_SOCKET_HANDLE_WAITER_WIN_H_

#include <vector>

#include "platform/base/error.h"
#include "platform/impl/socket_handle_waiter.h"

namespace openscreen {

class SocketHandleWaiterWin : public SocketHandleWaiter {
 public:
  explicit SocketHandleWaiterWin(ClockNowFunctionPtr now_function)
      : SocketHandleWaiter(now_function) {}
  ~SocketHandleWaiterWin() override = default;

 protected:
  ErrorOr<std::vector<HandleWithFlags>> AwaitSocketsReady(
      const std::vector<HandleWithFlags>& sockets,
      const Clock::duration& timeout) override;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_SOCKET_HANDLE_WAITER_WIN_H_
