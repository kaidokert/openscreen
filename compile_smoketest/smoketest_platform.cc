#include <iostream>

#include "platform/base/error.h"

void TestPlatform() {
  openscreen::Error e(openscreen::Error::Code::kNone, "No error");
  if (e.ok()) {
    std::cout << "Error is OK" << std::endl;
  }
}
