#include "gtest/gtest.h"
#include "platform/api/logging.h"
#include "util/osp_logging.h"

TEST(LoggingWinTest, HelloWorld) {
  // This should not crash.
  // Note: Output might not be visible in console unless implementation writes
  // to stderr/stdout.
  OSP_LOG_INFO << "Hello World from LoggingWinTest!";
}
