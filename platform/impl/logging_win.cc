#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <vector>

#include "platform/api/logging.h"
#include "platform/impl/logging.h"
#include "platform/impl/logging_test.h"

namespace openscreen {

namespace {

LogLevel g_log_level = LogLevel::kWarning;
std::vector<std::string>* g_log_buffer_for_test = nullptr;
std::mutex g_log_mutex;

}  // namespace

bool IsLoggingOn(LogLevel level, const std::string_view file) {
  return level >= g_log_level;
}

void LogWithLevel(LogLevel level,
                  const char* file,
                  int line,
                  std::stringstream message) {
  std::string formatted_message;
  if (g_log_buffer_for_test) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    formatted_message = "[" + std::string(file) + ":" + std::to_string(line) +
                        "] " + message.str() + "\n";
    g_log_buffer_for_test->push_back(formatted_message);
  } else {
    char log_message_buffer[1024];
    _snprintf_s(log_message_buffer, sizeof(log_message_buffer), _TRUNCATE,
                "[%s:%d] %s\n", file, line, message.str().c_str());
    formatted_message = log_message_buffer;
    OutputDebugStringA(formatted_message.c_str());
  }

  if (level == LogLevel::kFatal) {
    Break();
  }
}

void Break() {
  if (IsDebuggerPresent()) {
    __debugbreak();
  }
  abort();
}

void SetLogLevel(LogLevel level) {
  std::lock_guard<std::mutex> lock(g_log_mutex);
  g_log_level = level;
}

LogLevel GetLogLevel() {
  std::lock_guard<std::mutex> lock(g_log_mutex);
  return g_log_level;
}

void SetLogBufferForTest(std::vector<std::string>* messages) {
  std::lock_guard<std::mutex> lock(g_log_mutex);
  g_log_buffer_for_test = messages;
}

void LogTraceMessage(const std::string& message) {
  // For now, trace messages are treated as info logs.
  // This ensures they are captured by SetLogBufferForTest or
  // OutputDebugStringA. It's technically not ideal to re-route via OSP_LOG_INFO
  // because it adds file/line, but it works as a stub.
  OSP_LOG_INFO << message;
}

}  // namespace openscreen
