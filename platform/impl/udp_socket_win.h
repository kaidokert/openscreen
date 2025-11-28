#ifndef PLATFORM_IMPL_UDP_SOCKET_WIN_H_
#define PLATFORM_IMPL_UDP_SOCKET_WIN_H_

#include "platform/api/udp_socket.h"

namespace openscreen {

class UdpSocketWin : public UdpSocket {
 public:
  UdpSocketWin(TaskRunner& task_runner,
               Client* client,
               const IPEndpoint& local_endpoint);
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

 private:
  TaskRunner& task_runner_;
  Client* client_;
  IPEndpoint local_endpoint_;
  bool is_bound_ = false;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_UDP_SOCKET_WIN_H_
