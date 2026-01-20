// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_PLATFORM_CLIENT_WIN_H_
#define PLATFORM_IMPL_PLATFORM_CLIENT_WIN_H_

#include <winsock2.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#ifdef SendMessage
#undef SendMessage
#endif

#include "platform/api/time.h"
#include "platform/impl/socket_handle_waiter.h"
#include "platform/impl/task_runner.h"
#include "platform/impl/udp_socket_reader_win.h"

namespace openscreen {

class PlatformClientWin {
 public:
  static void Create(Clock::duration networking_operation_timeout,
                     std::unique_ptr<TaskRunnerImpl> task_runner);
  static void Create(Clock::duration networking_operation_timeout);
  static void ShutDown();

  static PlatformClientWin* GetInstance() { return instance_; }

  UdpSocketReaderWin* udp_socket_reader();
  TaskRunner& GetTaskRunner();

 protected:
  ~PlatformClientWin();

  static void SetInstance(PlatformClientWin* client);

 private:
  explicit PlatformClientWin(Clock::duration networking_operation_timeout);
  PlatformClientWin(Clock::duration networking_operation_timeout,
                    std::unique_ptr<TaskRunnerImpl> task_runner);

  PlatformClientWin(const PlatformClientWin&) = delete;
  PlatformClientWin& operator=(const PlatformClientWin&) = delete;

  void RunNetworkLoopUntilStopped();

  std::unique_ptr<TaskRunnerImpl> task_runner_;
  std::atomic_bool networking_loop_running_{true};
  Clock::duration networking_loop_timeout_;

  // Instance objects are created at runtime when they are first needed.
  std::once_flag waiter_initialization_;
  std::once_flag udp_socket_reader_initialization_;

  std::unique_ptr<SocketHandleWaiter> waiter_;
  std::unique_ptr<UdpSocketReaderWin> udp_socket_reader_;

  std::thread networking_loop_thread_;
  std::optional<std::thread> task_runner_thread_;

  static PlatformClientWin* instance_;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_PLATFORM_CLIENT_WIN_H_
