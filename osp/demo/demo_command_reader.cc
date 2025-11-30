// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/demo/demo_command_reader.h"

#include <iostream>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace openscreen::osp {

namespace {

CommandLineSplit SeparateCommandFromArguments(const std::string& line) {
  size_t split_index = line.find_first_of(' ');
  std::string command = line.substr(0, split_index);
  std::string argument_tail =
      split_index < line.size() ? line.substr(split_index + 1) : std::string();
  return {std::move(command), std::move(argument_tail)};
}

}  // namespace

#if defined(_WIN32)

DemoCommandReader::DemoCommandReader() {
  input_handle_ = GetStdHandle(STD_INPUT_HANDLE);
}

DemoCommandReader::~DemoCommandReader() = default;

CommandWaitResult DemoCommandReader::WaitForCommand(const bool& stop_flag) {
  HANDLE input = static_cast<HANDLE>(input_handle_);
  HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD original_mode = 0;
  GetConsoleMode(input, &original_mode);
  // Disable line input and echo so we can process events manually.
  // ENABLE_PROCESSED_INPUT ensures Ctrl-C is still handled by the system.
  SetConsoleMode(input, ENABLE_PROCESSED_INPUT);

  std::string buffer;
  while (true) {
    if (stop_flag) {
      SetConsoleMode(input, original_mode);
      return {true};
    }

    // Wait for input or timeout to check stop_flag.
    DWORD ret = WaitForSingleObject(input, 50);
    if (ret == WAIT_OBJECT_0) {
      DWORD num_events = 0;
      GetNumberOfConsoleInputEvents(input, &num_events);

      // Process all available events.
      for (DWORD i = 0; i < num_events; ++i) {
        INPUT_RECORD record;
        DWORD read = 0;
        if (!ReadConsoleInputA(input, &record, 1, &read) || read == 0) {
          break;
        }

        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
          char c = record.Event.KeyEvent.uChar.AsciiChar;
          if (c == '\r') {  // Enter
            WriteConsoleA(output, "\n", 1, NULL, NULL);
            SetConsoleMode(input, original_mode);
            return {false, SeparateCommandFromArguments(buffer)};
          } else if (c == '\b') {  // Backspace
            if (!buffer.empty()) {
              buffer.pop_back();
              // Erase character from console: Backspace, Space, Backspace.
              WriteConsoleA(output, "\b \b", 3, NULL, NULL);
            }
          } else if (c >= 32) {  // Printable characters
            buffer.push_back(c);
            WriteConsoleA(output, &c, 1, NULL, NULL);
          }
        }
      }
    }
  }
}

#else

DemoCommandReader::DemoCommandReader() : stdin_pollfd_{STDIN_FILENO, POLLIN} {}

DemoCommandReader::~DemoCommandReader() = default;

CommandWaitResult DemoCommandReader::WaitForCommand(const bool& stop_flag) {
  while (poll(&stdin_pollfd_, 1, 10) >= 0) {
    if (stop_flag) {
      return {true};
    }

    if (stdin_pollfd_.revents == 0) {
      continue;
    } else if (stdin_pollfd_.revents & (POLLERR | POLLHUP)) {
      return {true};
    }

    std::string line;
    if (!std::getline(std::cin, line)) {
      return {true};
    }

    return {false, SeparateCommandFromArguments(line)};
  }
  return {true};
}

#endif

}  // namespace openscreen::osp
