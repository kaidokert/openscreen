// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
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

const char* MapLogLevelName(LogLevel level) {
  switch (level) {
    case LogLevel::kVerbose:
      return "VERBOSE";
    case LogLevel::kInfo:
      return "INFO";
    case LogLevel::kWarning:
      return "WARNING";
    case LogLevel::kError:
      return "ERROR";
    case LogLevel::kFatal:
      return "FATAL";
  }
  return "UNKNOWN";
}

}  // namespace

bool IsLoggingOn(LogLevel level, const std::string_view file) {
  return level >= g_log_level;
}

void LogWithLevel(LogLevel level,
                  const char* file,
                  int line,
                  std::stringstream message) {
  std::string formatted_message;
  std::string level_name = MapLogLevelName(level);

  // Format: [LEVEL:file:line] message\n
  std::stringstream ss;
  ss << "[" << level_name << ":" << file << ":" << line << "] " << message.str()
     << "\n";
  formatted_message = ss.str();

  if (g_log_buffer_for_test) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    g_log_buffer_for_test->push_back(formatted_message);
  }

  if (!g_log_buffer_for_test || level == LogLevel::kFatal) {
    // Write to stderr for death tests and console output
    fputs(formatted_message.c_str(), stderr);
    fflush(stderr);  // Ensure it's flushed for death tests

    // Also write to debug output for VS
    OutputDebugStringA(formatted_message.c_str());
  }

  if (level == LogLevel::kFatal) {
    Break();
  }
}

[[noreturn]] void Break() {
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
