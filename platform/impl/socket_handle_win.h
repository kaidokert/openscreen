#ifndef PLATFORM_IMPL_SOCKET_HANDLE_WIN_H_
#define PLATFORM_IMPL_SOCKET_HANDLE_WIN_H_

#include <winsock2.h>

#include "platform/impl/socket_handle.h"

namespace openscreen {

struct SocketHandle {
  explicit SocketHandle(SOCKET descriptor);
  SOCKET handle;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_SOCKET_HANDLE_WIN_H_
