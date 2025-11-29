#include "platform/impl/udp_socket_win.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "platform/api/task_runner.h"
#include "platform/impl/platform_client_win.h"
#include "util/osp_logging.h"

namespace openscreen {

namespace {

constexpr int kMaxUdpBufferSize = 64 << 10;

ErrorOr<SOCKET> CreateNonBlockingUdpSocket(int domain) {
  SOCKET sock = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == INVALID_SOCKET) {
    return Error(Error::Code::kInitializationFailure,
                 std::to_string(WSAGetLastError()));
  }

  u_long mode = 1;
  if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
    closesocket(sock);
    return Error(Error::Code::kInitializationFailure,
                 std::to_string(WSAGetLastError()));
  }
  return sock;
}

}  // namespace

// static
ErrorOr<std::unique_ptr<UdpSocket>> UdpSocket::Create(
    TaskRunner& task_runner,
    Client* client,
    const IPEndpoint& local_endpoint) {
  int domain = (local_endpoint.address.version() == UdpSocket::Version::kV6)
                   ? AF_INET6
                   : AF_INET;
  auto result = CreateNonBlockingUdpSocket(domain);
  if (result.is_error()) {
    return result.error();
  }

  return std::unique_ptr<UdpSocket>(
      new UdpSocketWin(task_runner, client, SocketHandle(result.value()),
                       local_endpoint, PlatformClientWin::GetInstance()));
}

UdpSocketWin::UdpSocketWin(TaskRunner& task_runner,
                           Client* client,
                           SocketHandle handle,
                           const IPEndpoint& local_endpoint,
                           PlatformClientWin* platform_client)
    : task_runner_(task_runner),
      client_(client),
      local_endpoint_(local_endpoint),
      handle_(handle),
      platform_client_(platform_client) {
  if (handle_.handle != INVALID_SOCKET && platform_client_) {
    platform_client_->udp_socket_reader()->OnCreate(this);
  }
}

UdpSocketWin::~UdpSocketWin() {
  Close();
}

bool UdpSocketWin::IsIPv4() const {
  return local_endpoint_.address.IsV4();
}

bool UdpSocketWin::IsIPv6() const {
  return local_endpoint_.address.IsV6();
}

IPEndpoint UdpSocketWin::GetLocalEndpoint() const {
  if (local_endpoint_.port == 0 && handle_.handle != INVALID_SOCKET) {
    sockaddr_storage addr;
    int len = sizeof(addr);
    if (getsockname(handle_.handle, (sockaddr*)&addr, &len) == 0) {
      if (addr.ss_family == AF_INET) {
        sockaddr_in* sin = (sockaddr_in*)&addr;
        local_endpoint_.port = ntohs(sin->sin_port);
        local_endpoint_.address =
            IPAddress(IPAddress::Version::kV4, (uint8_t*)&sin->sin_addr);
      } else if (addr.ss_family == AF_INET6) {
        sockaddr_in6* sin6 = (sockaddr_in6*)&addr;
        local_endpoint_.port = ntohs(sin6->sin6_port);
        local_endpoint_.address =
            IPAddress(IPAddress::Version::kV6, (uint8_t*)&sin6->sin6_addr);
      }
    }
  }
  return local_endpoint_;
}

void UdpSocketWin::Bind() {
  if (handle_.handle == INVALID_SOCKET) {
    OnError(Error::Code::kSocketClosedFailure);
    return;
  }

  BOOL opt = TRUE;
  setsockopt(handle_.handle, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt,
             sizeof(opt));

  int res = SOCKET_ERROR;
  if (IsIPv4()) {
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(local_endpoint_.port);
    local_endpoint_.address.CopyToV4((uint8_t*)&sin.sin_addr);
    res = bind(handle_.handle, (sockaddr*)&sin, sizeof(sin));
  } else {
    sockaddr_in6 sin6;
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(local_endpoint_.port);
    local_endpoint_.address.CopyToV6((uint8_t*)&sin6.sin6_addr);
    res = bind(handle_.handle, (sockaddr*)&sin6, sizeof(sin6));
  }

  if (res == SOCKET_ERROR) {
    OnError(Error::Code::kSocketBindFailure);
    return;
  }

  is_bound_ = true;
  if (client_) {
    task_runner_.PostTask([weak_this = weak_factory_.GetWeakPtr()] {
      if (auto* self = weak_this.get()) {
        if (self->client_)
          self->client_->OnBound(self);
      }
    });
  }
}

void UdpSocketWin::SetMulticastOutboundInterface(
    NetworkInterfaceIndex ifindex) {
  if (handle_.handle == INVALID_SOCKET)
    return;

  if (IsIPv4()) {
    // TODO: Implement proper IPv4 interface selection using IP
    OSP_UNIMPLEMENTED();
  } else {
    DWORD index = static_cast<DWORD>(ifindex);
    setsockopt(handle_.handle, IPPROTO_IPV6, IPV6_MULTICAST_IF,
               (const char*)&index, sizeof(index));
  }
}

void UdpSocketWin::JoinMulticastGroup(const IPAddress& address,
                                      NetworkInterfaceIndex ifindex) {
  if (handle_.handle == INVALID_SOCKET)
    return;

  if (IsIPv4()) {
    struct ip_mreq mreq;
    address.CopyToV4((uint8_t*)&mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(handle_.handle, IPPROTO_IP, IP_ADD_MEMBERSHIP,
               (const char*)&mreq, sizeof(mreq));
  } else {
    struct ipv6_mreq mreq;
    address.CopyToV6((uint8_t*)&mreq.ipv6mr_multiaddr);
    mreq.ipv6mr_interface = static_cast<unsigned int>(ifindex);
    setsockopt(handle_.handle, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
               (const char*)&mreq, sizeof(mreq));
  }
}

void UdpSocketWin::SendMessage(ByteView data, const IPEndpoint& dest) {
  if (handle_.handle == INVALID_SOCKET) {
    if (client_)
      client_->OnSendError(this, Error::Code::kSocketClosedFailure);
    return;
  }

  const char* buf = reinterpret_cast<const char*>(data.data());
  int len = static_cast<int>(data.size());

  int res = SOCKET_ERROR;

  if (dest.address.version() == UdpSocket::Version::kV4) {
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(dest.port);
    dest.address.CopyToV4((uint8_t*)&sin.sin_addr);
    res = sendto(handle_.handle, buf, len, 0, (sockaddr*)&sin, sizeof(sin));
  } else {
    sockaddr_in6 sin6;
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(dest.port);
    dest.address.CopyToV6((uint8_t*)&sin6.sin6_addr);
    res = sendto(handle_.handle, buf, len, 0, (sockaddr*)&sin6, sizeof(sin6));
  }

  if (res == SOCKET_ERROR) {
    if (client_) {
      client_->OnSendError(this, Error::Code::kSocketSendFailure);
    }
  }
}

void UdpSocketWin::SetDscp(DscpMode state) {
  // Stub
  OSP_UNIMPLEMENTED();
}

void UdpSocketWin::ReceiveMessage() {
  if (handle_.handle == INVALID_SOCKET)
    return;

  char buffer[kMaxUdpBufferSize];
  sockaddr_storage src_addr;
  int src_len = sizeof(src_addr);

  int bytes = recvfrom(handle_.handle, buffer, sizeof(buffer), 0,
                       (sockaddr*)&src_addr, &src_len);

  if (bytes == SOCKET_ERROR) {
    // WSAGetLastError could be checked here
    return;
  }

  UdpPacket packet(bytes);
  if (bytes > 0) {
    memcpy(packet.data(), buffer, bytes);
  }

  IPEndpoint source_endpoint;
  if (src_addr.ss_family == AF_INET) {
    sockaddr_in* sin = (sockaddr_in*)&src_addr;
    source_endpoint.port = ntohs(sin->sin_port);
    source_endpoint.address =
        IPAddress(IPAddress::Version::kV4, (uint8_t*)&sin->sin_addr);
  } else if (src_addr.ss_family == AF_INET6) {
    sockaddr_in6* sin6 = (sockaddr_in6*)&src_addr;
    source_endpoint.port = ntohs(sin6->sin6_port);
    source_endpoint.address =
        IPAddress(IPAddress::Version::kV6, (uint8_t*)&sin6->sin6_addr);
  }
  packet.set_source(source_endpoint);

  auto shared_packet = std::make_shared<UdpPacket>(std::move(packet));
  task_runner_.PostTask(
      [weak_this = weak_factory_.GetWeakPtr(), shared_packet]() {
        if (auto* self = weak_this.get()) {
          if (self->client_) {
            self->client_->OnRead(self, std::move(*shared_packet));
          }
        }
      });
}

void UdpSocketWin::OnError(Error::Code error) {
  Close();
  if (client_) {
    client_->OnError(this, Error(error, std::to_string(WSAGetLastError())));
  }
}

void UdpSocketWin::Close() {
  if (handle_.handle != INVALID_SOCKET) {
    // Notify the reader that the socket handle is about to be closed.
    if (platform_client_) {
      platform_client_->udp_socket_reader()->OnDestroy(this);
    }
    closesocket(handle_.handle);
    handle_.handle = INVALID_SOCKET;
  }
}

}  // namespace openscreen
