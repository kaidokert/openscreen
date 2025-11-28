#include "platform/impl/socket_handle_waiter.h"

namespace openscreen {

ErrorOr<std::vector<SocketHandleWaiter::HandleWithFlags>>
SocketHandleWaiter::AwaitSocketsReady(
    const std::vector<SocketHandleWaiter::HandleWithFlags>& handles,
    const Clock::duration& timeout) {
  // TODO: Implement using select() or WSAWaitForMultipleEvents
  return std::vector<SocketHandleWaiter::HandleWithFlags>{};
}

}  // namespace openscreen
