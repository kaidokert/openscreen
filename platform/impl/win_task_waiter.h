#ifndef PLATFORM_IMPL_WIN_TASK_WAITER_H_
#define PLATFORM_IMPL_WIN_TASK_WAITER_H_

#include <windows.h>

#include "platform/impl/task_runner.h"

namespace openscreen {

class WinTaskWaiter : public TaskRunnerImpl::TaskWaiter {
 public:
  WinTaskWaiter();
  ~WinTaskWaiter() override;

  // TaskRunnerImpl::TaskWaiter overrides.
  Error WaitForTaskToBePosted(Clock::duration timeout) override;
  void OnTaskPosted() override;

 private:
  // Event handle to signal that a task has been posted.
  HANDLE task_posted_event_;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_WIN_TASK_WAITER_H_
