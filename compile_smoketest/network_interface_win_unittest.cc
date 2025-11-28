#include "gtest/gtest.h"
#include "platform/api/network_interface.h"

TEST(NetworkInterfaceWinTest, GetInterfaces) {
  auto interfaces = openscreen::GetNetworkInterfaces();
  // It shouldn't crash.
}
