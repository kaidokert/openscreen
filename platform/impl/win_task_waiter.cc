#include "platform/impl/win_task_waiter.h"

#include <algorithm>

#include "util/osp_logging.h"

namespace openscreen {

WinTaskWaiter::WinTaskWaiter()
    : task_posted_event_(CreateEvent(nullptr, FALSE, FALSE, nullptr)) {
  OSP_CHECK(task_posted_event_ != nullptr) << "Failed to create event.";
}

WinTaskWaiter::~WinTaskWaiter() {
  CloseHandle(task_posted_event_);
}

Error WinTaskWaiter::WaitForTaskToBePosted(Clock::duration timeout) {
  DWORD milliseconds =
      (timeout == Clock::duration::max())
          ? INFINITE
          : std::max(0LL, timeout.count() /
                              1000LL);  // Convert microseconds to milliseconds

  DWORD result = WaitForSingleObject(task_posted_event_, milliseconds);
  if (result == WAIT_OBJECT_0) {
    return Error::None();
  } else if (result == WAIT_TIMEOUT) {
    return Error::Code::kTimeout;
  } else {
    // Other error, e.g., WAIT_ABANDONED, or an actual error code.
    OSP_LOG_ERROR << "WaitForTaskToBePosted failed with error: "
                  << GetLastError();
    return Error::Code::kUnknownError;
  }
}

void WinTaskWaiter::OnTaskPosted() {
  SetEvent(task_posted_event_);
}

}  // namespace openscreen
