#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>

#include "gtest/gtest.h"
#include "platform/api/time.h"
#include "platform/impl/task_runner.h"
#include "platform/impl/win_task_waiter.h"
#include "util/osp_logging.h"

using namespace openscreen;

TEST(TaskRunnerWinTest, RunsTasks) {
  WinTaskWaiter waiter;
  TaskRunnerImpl runner(Clock::now, &waiter);

  std::atomic_bool ran = false;
  OSP_LOG_INFO << "Posting task...";
  runner.PostTask([&ran, &runner] {
    OSP_LOG_INFO << "Task running...";
    ran = true;
    runner.RequestStopSoon();
    OSP_LOG_INFO << "RequestStopSoon called.";
  });

  // Watchdog thread
  std::thread watchdog([]() {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    OSP_LOG_ERROR << "Test timed out!";
    std::exit(1);
  });
  watchdog.detach();

  OSP_LOG_INFO << "Starting RunUntilStopped...";
  runner.RunUntilStopped();
  OSP_LOG_INFO << "RunUntilStopped returned.";

  EXPECT_TRUE(ran);
}
