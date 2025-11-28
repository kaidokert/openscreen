#include <windows.h>

#include <chrono>

#include "platform/api/time.h"

namespace openscreen {

namespace {

// January 1, 1970, in Windows' FILETIME format (100-nanosecond intervals
// since January 1, 1601).
constexpr uint64_t kUnixEpochStartTicks = 116444736000000000ULL;

// Converts a Windows FILETIME to a std::chrono::time_point representing
// seconds since the Unix epoch.
std::chrono::time_point<Clock> FileTimeToTimePoint(FILETIME ft) {
  uint64_t file_time_value =
      (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
  // Convert 100-nanosecond intervals since 1601 to microseconds since 1970.
  uint64_t microseconds_since_epoch =
      (file_time_value - kUnixEpochStartTicks) / 10;
  return Clock::time_point(std::chrono::duration_cast<Clock::duration>(
      std::chrono::microseconds(microseconds_since_epoch)));
}

}  // namespace

// static
Clock::time_point Clock::now() noexcept {
  // Using std::chrono::high_resolution_clock, which is typically
  // QueryPerformanceCounter on Windows for high resolution.
  return Clock::time_point(std::chrono::duration_cast<Clock::duration>(
      std::chrono::high_resolution_clock::now().time_since_epoch()));
}

std::chrono::seconds GetWallTimeSinceUnixEpoch() noexcept {
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  return std::chrono::duration_cast<std::chrono::seconds>(
      FileTimeToTimePoint(ft).time_since_epoch());
}

}  // namespace openscreen
