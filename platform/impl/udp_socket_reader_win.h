// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_UDP_SOCKET_READER_WIN_H_
#define PLATFORM_IMPL_UDP_SOCKET_READER_WIN_H_

#include <map>
#include <mutex>
#include <vector>

#include "platform/api/task_runner.h"
#include "platform/api/time.h"
#include "platform/impl/socket_handle.h"
#include "platform/impl/socket_handle_waiter.h"
#include "platform/impl/udp_socket_win.h"

namespace openscreen {

class UdpSocketReaderWin : public SocketHandleWaiter::Subscriber {
 public:
  using SocketHandleRef = SocketHandleWaiter::SocketHandleRef;

  explicit UdpSocketReaderWin(SocketHandleWaiter& waiter);
  ~UdpSocketReaderWin() override;

  void OnCreate(UdpSocketWin* socket);
  void OnDestroy(UdpSocketWin* socket);

  // SocketHandleWaiter::Subscriber overrides.
  void ProcessReadyHandle(SocketHandleRef handle, uint32_t flags) override;
  bool HasPendingWrite(SocketHandleRef handle) override;

  OSP_DISALLOW_COPY_AND_ASSIGN(UdpSocketReaderWin);

 protected:
  bool IsMappedReadForTesting(UdpSocketWin* socket) const;

 private:
  void OnDelete(UdpSocketWin* socket,
                bool disable_locking_for_testing = false);

  std::vector<UdpSocketWin*> sockets_;
  mutable std::mutex mutex_;
  SocketHandleWaiter& waiter_;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_UDP_SOCKET_READER_WIN_H_
