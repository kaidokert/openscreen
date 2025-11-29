#include <cstddef>  // For size_t, which ssize_t is related to.

#include "platform/base/macros.h"

#if defined(_WIN32)
// This function will only be compiled on Windows.
// It tries to use ssize_t without any custom definition.
// If ssize_t is not natively defined on Windows, this should fail compilation.
void TestSsizeTOnWindows() {
  ssize_t value = 0;
  (void)value;
}
#endif

int main() {
  // Call the function on Windows to ensure it's instantiated
#if defined(_WIN32)
  TestSsizeTOnWindows();
#endif
  return 0;
}
