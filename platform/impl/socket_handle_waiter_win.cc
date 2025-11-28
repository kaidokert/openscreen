#include "platform/impl/socket_handle_waiter.h"
#include "platform/impl/socket_handle_waiter_win.h"

#include <winsock2.h>
#include <thread>

#include "platform/impl/socket_handle_win.h"
#include "util/osp_logging.h"

namespace openscreen {

ErrorOr<std::vector<SocketHandleWaiter::HandleWithFlags>>
SocketHandleWaiterWin::AwaitSocketsReady(
    const std::vector<SocketHandleWaiter::HandleWithFlags>& handles,
    const Clock::duration& timeout) {
  if (handles.empty()) {
    std::this_thread::sleep_for(timeout);
    return std::vector<SocketHandleWaiter::HandleWithFlags>{};
  }

  if (handles.size() > FD_SETSIZE) {
    OSP_LOG_ERROR << "Too many sockets for select(): " << handles.size();
    return Error::Code::kSocketOptionSettingFailure;
  }

  fd_set read_fds;
  fd_set write_fds;
  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);

  for (const auto& handle_with_flags : handles) {
    SOCKET s = handle_with_flags.handle.get().handle;
    if (handle_with_flags.flags & Flags::kReadable) {
      FD_SET(s, &read_fds);
    }
    if (handle_with_flags.flags & Flags::kWritable) {
      FD_SET(s, &write_fds);
    }
  }

  struct timeval tv;
  auto micros = std::chrono::duration_cast<std::chrono::microseconds>(timeout).count();
  tv.tv_sec = static_cast<long>(micros / 1000000);
  tv.tv_usec = static_cast<long>(micros % 1000000);

  int res = select(0, &read_fds, &write_fds, nullptr, &tv);

  if (res == SOCKET_ERROR) {
    int err = WSAGetLastError();
    OSP_LOG_ERROR << "select failed: " << err;
    return Error::Code::kSocketAcceptFailure;
  }

  std::vector<SocketHandleWaiter::HandleWithFlags> ready_handles;
  if (res > 0) {
    for (const auto& handle_with_flags : handles) {
      SOCKET s = handle_with_flags.handle.get().handle;
      uint32_t ready_flags = 0;
      if (FD_ISSET(s, &read_fds)) {
        ready_flags |= Flags::kReadable;
      }
      if (FD_ISSET(s, &write_fds)) {
        ready_flags |= Flags::kWritable;
      }
      if (ready_flags) {
        ready_handles.push_back({handle_with_flags.handle, ready_flags});
      }
    }
  }

  return ready_handles;
}

}  // namespace openscreen