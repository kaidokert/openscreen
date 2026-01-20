// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/network_interface.h"

// clang-format off
#include <winsock2.h>
#include <iphlpapi.h>
#include <windows.h>
#include <ws2tcpip.h>
// clang-format on

#include <vector>

#include "util/osp_logging.h"

namespace openscreen {

namespace {

InterfaceInfo::Type GetInterfaceType(IFTYPE if_type) {
  switch (if_type) {
    case IF_TYPE_SOFTWARE_LOOPBACK:
      return InterfaceInfo::Type::kLoopback;
    case IF_TYPE_ETHERNET_CSMACD:
      return InterfaceInfo::Type::kEthernet;
    case IF_TYPE_IEEE80211:
      return InterfaceInfo::Type::kWifi;
    default:
      return InterfaceInfo::Type::kOther;
  }
}

IPAddress GetIPAddressFromSockAddr(const sockaddr* sa) {
  if (sa->sa_family == AF_INET) {
    const auto* sin = reinterpret_cast<const sockaddr_in*>(sa);
    const uint8_t* b = reinterpret_cast<const uint8_t*>(&sin->sin_addr);
    return IPAddress(b[0], b[1], b[2], b[3]);
  } else if (sa->sa_family == AF_INET6) {
    const auto* sin6 = reinterpret_cast<const sockaddr_in6*>(sa);
    // In Windows, sin6_addr is a union, but u.Byte is an array of 16 bytes.
    const uint8_t* b = sin6->sin6_addr.u.Byte;
    // IPAddress expects hextets for IPv6 constructor used with 16 bytes array?
    // Actually IPAddress(Version::kV6, const uint8_t* bytes) exists.
    return IPAddress(IPAddress::Version::kV6, b);
  }
  return {};  // Invalid/Unknown
}

}  // namespace

std::vector<InterfaceInfo> GetNetworkInterfaces() {
  std::vector<InterfaceInfo> interfaces;
  ULONG out_buf_len = 15000;
  std::vector<unsigned char> out_buf(out_buf_len);
  PIP_ADAPTER_ADDRESSES addresses =
      reinterpret_cast<PIP_ADAPTER_ADDRESSES>(out_buf.data());

  // GetAdaptersAddresses generally recommends running it once to get size,
  // but a 15k buffer is usually enough. Loop to be safe.
  ULONG ret =
      GetAdaptersAddresses(AF_UNSPEC,
                           GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                               GAA_FLAG_SKIP_DNS_SERVER,
                           nullptr, addresses, &out_buf_len);

  if (ret == ERROR_BUFFER_OVERFLOW) {
    out_buf.resize(out_buf_len);
    addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(out_buf.data());
    ret = GetAdaptersAddresses(AF_UNSPEC,
                               GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                                   GAA_FLAG_SKIP_DNS_SERVER,
                               nullptr, addresses, &out_buf_len);
  }

  if (ret != NO_ERROR) {
    OSP_LOG_ERROR << "GetAdaptersAddresses failed: " << ret;
    return interfaces;
  }

  for (PIP_ADAPTER_ADDRESSES curr_address = addresses; curr_address != nullptr;
       curr_address = curr_address->Next) {
    InterfaceInfo info;

    // Windows IfIndex is unsigned long, Open Screen uses InterfaceIndex (int
    // usually). Ensure we handle casting safely or accept truncation if indices
    // are small. On Windows, IfIndex is usually small index.
    info.index = static_cast<NetworkInterfaceIndex>(curr_address->IfIndex);

    info.type = GetInterfaceType(curr_address->IfType);

    if (curr_address->PhysicalAddressLength > 0 &&
        curr_address->PhysicalAddressLength <= sizeof(info.hardware_address)) {
      std::copy(
          curr_address->PhysicalAddress,
          curr_address->PhysicalAddress + curr_address->PhysicalAddressLength,
          std::back_inserter(info.hardware_address));
    }

    info.name = curr_address->AdapterName ? curr_address->AdapterName : "";

    const PIP_ADAPTER_UNICAST_ADDRESS unicast =
        curr_address->FirstUnicastAddress;
    for (auto* addr = unicast; addr != nullptr; addr = addr->Next) {
      if (const auto* sockaddr = addr->Address.lpSockaddr) {
        const IPAddress ip = GetIPAddressFromSockAddr(sockaddr);
        // Skip invalid/unknown addresses
        if (ip.version() == IPAddress::Version::kV4 ||
            ip.version() == IPAddress::Version::kV6) {
          // OnLinkPrefixLength available since Windows Vista.
          const int8_t prefix_length =
              static_cast<int8_t>(addr->OnLinkPrefixLength);
          info.addresses.emplace_back(ip, prefix_length);
        }
      }
    }

    interfaces.push_back(info);
  }

  return interfaces;
}

}  // namespace openscreen
