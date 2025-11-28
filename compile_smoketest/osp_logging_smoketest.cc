#include "platform/api/logging.h"
#include "util/osp_logging.h"
#include "util/trace_logging.h"

#include <string>
#include <vector>
#include <map>
#include <iostream>

// Custom macro to mimic OSP_CHECK for testing.
#define WASP_LOG_IS_ON(level_enum) \
  ::openscreen::IsLoggingOn(::openscreen::LogLevel::level_enum, __FILE__)

#define WASP_LOG_STREAM(level_enum) \
  ::openscreen::internal::LogMessage(::openscreen::LogLevel::level_enum, __FILE__, __LINE__)

#define WASP_LAZY_STREAM(condition, stream) \
  !(condition) ? (void)0 : ::openscreen::internal::Voidify() & (stream)

#define WASP_LOG_INFO WASP_LAZY_STREAM(WASP_LOG_IS_ON(kInfo), WASP_LOG_STREAM(kInfo))
#define WASP_LOG_WARN WASP_LAZY_STREAM(WASP_LOG_IS_ON(kWarning), WASP_LOG_STREAM(kWarning))
#define WASP_LOG_ERROR WASP_LAZY_STREAM(WASP_LOG_IS_ON(kError), WASP_LOG_STREAM(kError))
#define WASP_LOG_FATAL WASP_LAZY_STREAM(WASP_LOG_IS_ON(kFatal), WASP_LOG_STREAM(kFatal))

#define WASP_CHECK(condition) \
  WASP_LOG_IF(FATAL, !(condition)) << "WASP_CHECK(" << #condition << ") failed: "

#define WASP_LOG_IF(level, condition) \
  !(condition) ? (void)0 : WASP_LOG_##level


namespace openscreen::osp {

// Mock class structure similar to DemoReceiverObserver
class MockReceiverObserver {
 public:
  std::string GetInstanceName(const std::string& safe_instance_name) {
    std::map<std::string, std::string> safe_instance_names_;
    WASP_CHECK(safe_instance_names_.find(safe_instance_name) != \
               safe_instance_names_.end()) \
        << safe_instance_name << " not found in map";
    return safe_instance_names_[safe_instance_name];
  }
};

} // namespace openscreen::osp

// Minimal main function to allow compilation as an executable
int main() {
    openscreen::osp::MockReceiverObserver obs;
    // This will hit the WASP_CHECK, but the goal is to make the macro itself compile.
    // obs.GetInstanceName("test"); 
    return 0;
}
