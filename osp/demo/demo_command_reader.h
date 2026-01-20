// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_DEMO_DEMO_COMMAND_READER_H_
#define OSP_DEMO_DEMO_COMMAND_READER_H_

#include <string>

#if !defined(_WIN32)
#include <poll.h>
#endif

namespace openscreen::osp {

struct CommandLineSplit {
  std::string command;
  std::string argument_tail;
};

struct CommandWaitResult {
  bool done;
  CommandLineSplit command_line;
};

class DemoCommandReader {
 public:
  DemoCommandReader();
  ~DemoCommandReader();

  CommandWaitResult WaitForCommand(const bool& stop_flag);

 private:
#if defined(_WIN32)
  void* input_handle_;  // HANDLE is void*
#else
  struct pollfd stdin_pollfd_;
#endif
};

}  // namespace openscreen::osp

#endif  // OSP_DEMO_DEMO_COMMAND_READER_H_
