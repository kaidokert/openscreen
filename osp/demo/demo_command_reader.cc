// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/demo/demo_command_reader.h"

#include <iostream>

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
  while (true) {
    if (stop_flag) {
      return {true};
    }
    DWORD ret = WaitForSingleObject(static_cast<HANDLE>(input_handle_), 10);
    if (ret == WAIT_OBJECT_0) {
      std::string line;
      if (!std::getline(std::cin, line)) {
        return {true};
      }
      return {false, SeparateCommandFromArguments(line)};
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
