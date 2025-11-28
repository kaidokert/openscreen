#include "util/osp_logging.h"

namespace openscreen::internal {

LogMessage::LogMessage(LogLevel level, const char* file, int line)
    : level_(level), file_(file), line_(line) {}

LogMessage::~LogMessage() {
  LogWithLevel(level_, file_, line_, std::move(stream_));
  if (level_ == LogLevel::kFatal) {
    Break();
  }
}

}  // namespace openscreen::internal
