// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_UDP_SOCKET_WIN_H_
#define PLATFORM_IMPL_UDP_SOCKET_WIN_H_

#include <winsock2.h>

#ifdef SendMessage
#undef SendMessage
#endif

#include "platform/api/udp_socket.h"
#include "platform/impl/socket_handle_win.h"
#include "util/weak_ptr.h"

namespace openscreen {

class UdpSocketReaderWin;
class PlatformClientWin;  // Forward declaration

class UdpSocketWin : public UdpSocket {
 public:
  UdpSocketWin(TaskRunner& task_runner,
               Client* client,
               SocketHandle handle,
               const IPEndpoint& local_endpoint,
               PlatformClientWin* platform_client = nullptr);
  ~UdpSocketWin() override;

  // UdpSocket overrides.
  bool IsIPv4() const override;
  bool IsIPv6() const override;
  IPEndpoint GetLocalEndpoint() const override;
  void Bind() override;
  void SetMulticastOutboundInterface(NetworkInterfaceIndex ifindex) override;
  void JoinMulticastGroup(const IPAddress& address,
                          NetworkInterfaceIndex ifindex) override;
  void SendMessage(ByteView data, const IPEndpoint& dest) override;
  void SetDscp(DscpMode state) override;

  const SocketHandle& GetHandle() const { return handle_; }

  // Called by UdpSocketReaderWin.
  void ReceiveMessage();

 private:
  void OnError(Error::Code error);
  void Close();

  TaskRunner& task_runner_;
  Client* client_;
  mutable IPEndpoint local_endpoint_;
  SocketHandle handle_;
  bool is_bound_ = false;

  WeakPtrFactory<UdpSocketWin> weak_factory_{this};

  PlatformClientWin* const platform_client_;

  friend class UdpSocketReaderWin;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_UDP_SOCKET_WIN_H_
