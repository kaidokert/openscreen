#include "platform/impl/udp_socket_win.h"

#include <memory>

#include "platform/api/task_runner.h"
#include "util/osp_logging.h"

namespace openscreen {

// static
ErrorOr<std::unique_ptr<UdpSocket>> UdpSocket::Create(
    TaskRunner& task_runner,
    Client* client,
    const IPEndpoint& local_endpoint) {
  return std::unique_ptr<UdpSocket>(
      new UdpSocketWin(task_runner, client, local_endpoint));
}

UdpSocketWin::UdpSocketWin(TaskRunner& task_runner,
                           Client* client,
                           const IPEndpoint& local_endpoint)
    : task_runner_(task_runner),
      client_(client),
      local_endpoint_(local_endpoint) {}

UdpSocketWin::~UdpSocketWin() {}

bool UdpSocketWin::IsIPv4() const {
  return local_endpoint_.address.IsV4();
}

bool UdpSocketWin::IsIPv6() const {
  return local_endpoint_.address.IsV6();
}

IPEndpoint UdpSocketWin::GetLocalEndpoint() const {
  return local_endpoint_;
}

void UdpSocketWin::Bind() {
  is_bound_ = true;
  // TODO: Real implementation with Winsock2.
  if (client_) {
    task_runner_.PostTask([client = client_, this] { client->OnBound(this); });
  }
}

void UdpSocketWin::SetMulticastOutboundInterface(
    NetworkInterfaceIndex ifindex) {
  OSP_UNIMPLEMENTED();
}

void UdpSocketWin::JoinMulticastGroup(const IPAddress& address,
                                      NetworkInterfaceIndex ifindex) {
  OSP_UNIMPLEMENTED();
}

void UdpSocketWin::SendMessage(ByteView data, const IPEndpoint& dest) {
  OSP_UNIMPLEMENTED();
}

void UdpSocketWin::SetDscp(DscpMode state) {
  OSP_UNIMPLEMENTED();
}

}  // namespace openscreen
