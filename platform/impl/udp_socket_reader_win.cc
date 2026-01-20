// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/udp_socket_reader_win.h"

#include <algorithm>

#include "util/osp_logging.h"

namespace openscreen {

UdpSocketReaderWin::UdpSocketReaderWin(SocketHandleWaiter& waiter)
    : waiter_(waiter) {}

UdpSocketReaderWin::~UdpSocketReaderWin() = default;

void UdpSocketReaderWin::OnCreate(UdpSocketWin* socket) {
  std::unique_lock<std::mutex> lock(mutex_);
  sockets_.push_back(socket);
  waiter_.Subscribe(this, socket->GetHandle(),
                    SocketHandleWaiter::Flags::kReadable);
}

void UdpSocketReaderWin::OnDestroy(UdpSocketWin* socket) {
  OnDelete(socket);
}

void UdpSocketReaderWin::OnDelete(UdpSocketWin* socket,
                                  bool disable_locking_for_testing) {
  if (!disable_locking_for_testing) {
    std::unique_lock<std::mutex> lock(mutex_);
  }

  auto it = std::remove(sockets_.begin(), sockets_.end(), socket);
  OSP_CHECK(it != sockets_.end()) << "UdpSocket not found for OnDestroy.";
  sockets_.erase(it, sockets_.end());

  waiter_.Unsubscribe(this, socket->GetHandle());
  waiter_.OnHandleDeletion(this, socket->GetHandle(),
                           disable_locking_for_testing);
}

void UdpSocketReaderWin::ProcessReadyHandle(SocketHandleRef handle,
                                            uint32_t flags) {
  OSP_DCHECK(flags & SocketHandleWaiter::Flags::kReadable);

  UdpSocketWin* target_socket = nullptr;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto* socket : sockets_) {
      if (socket->GetHandle() == handle.get()) {
        target_socket = socket;
        break;
      }
    }
  }

  if (target_socket) {
    target_socket->ReceiveMessage();
  }
}

bool UdpSocketReaderWin::HasPendingWrite(SocketHandleRef handle) {
  // The UdpSocketReaderWin only subscribes to read events, so it never has
  // pending writes.
  return false;
}

bool UdpSocketReaderWin::IsMappedReadForTesting(UdpSocketWin* socket) const {
  std::unique_lock<std::mutex> lock(mutex_);
  auto it = std::find(sockets_.begin(), sockets_.end(), socket);
  return it != sockets_.end();
}

}  // namespace openscreen
