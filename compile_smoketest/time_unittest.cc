#include "platform/api/time.h"

#include <chrono>
#include <thread>

#include "gtest/gtest.h"

using openscreen::Clock;

TEST(TimeTest, Monotonicity) {
  auto start = Clock::now();
  // Sleep a tiny bit.
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  auto end = Clock::now();

  EXPECT_GE(end, start);
  // On some systems/VMs, 10ms might not register if resolution is low, but
  // high_resolution_clock should catch it.
  EXPECT_GT(end, start);
}

TEST(TimeTest, WallTime) {
  auto wall = openscreen::GetWallTimeSinceUnixEpoch();
  EXPECT_GT(wall.count(), 1600000000);  // Sanity check > year 2020
}
