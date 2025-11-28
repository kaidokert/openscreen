// clang-format off
#include "platform/api/network_interface.h"

#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

// clang-format on

#include <vector>

#include "util/osp_logging.h"

namespace openscreen {

std::vector<InterfaceInfo> GetNetworkInterfaces() {
  std::vector<InterfaceInfo> interfaces;
  ULONG out_buf_len = 15000;
  std::vector<unsigned char> out_buf(out_buf_len);
  PIP_ADAPTER_ADDRESSES pAddresses =
      reinterpret_cast<PIP_ADAPTER_ADDRESSES>(out_buf.data());

  ULONG ret =
      GetAdaptersAddresses(AF_UNSPEC,
                           GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                               GAA_FLAG_SKIP_DNS_SERVER,
                           NULL, pAddresses, &out_buf_len);

  if (ret == ERROR_BUFFER_OVERFLOW) {
    out_buf.resize(out_buf_len);
    pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(out_buf.data());
    ret = GetAdaptersAddresses(AF_UNSPEC,
                               GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                                   GAA_FLAG_SKIP_DNS_SERVER,
                               NULL, pAddresses, &out_buf_len);
  }

  if (ret == NO_ERROR) {
    // Iteration logic would go here.
    // For now we just verified the API call works.
  } else {
    OSP_LOG_ERROR << "GetAdaptersAddresses failed: " << ret;
  }

  return interfaces;
}

}  // namespace openscreen