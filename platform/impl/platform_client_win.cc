// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/platform_client_win.h"

#include <winsock2.h>

#include <cstdint>
#include <utility>

#include "platform/impl/socket_handle_waiter_win.h"
#include "platform/impl/task_runner.h"
#include "util/osp_logging.h"

namespace openscreen {

// static
PlatformClientWin* PlatformClientWin::instance_ = nullptr;

// static
void PlatformClientWin::Create(Clock::duration networking_operation_timeout,
                               std::unique_ptr<TaskRunnerImpl> task_runner) {
  OSP_DCHECK(!instance_);
  auto* client = new PlatformClientWin(networking_operation_timeout,
                                       std::move(task_runner));
  SetInstance(client);
}

// static
void PlatformClientWin::Create(Clock::duration networking_operation_timeout) {
  OSP_DCHECK(!instance_);
  auto* client = new PlatformClientWin(networking_operation_timeout);
  SetInstance(client);
}

// static
void PlatformClientWin::ShutDown() {
  OSP_DCHECK(instance_);
  delete instance_;
  SetInstance(nullptr);
}

// static
void PlatformClientWin::SetInstance(PlatformClientWin* client) {
  instance_ = client;
}

PlatformClientWin::PlatformClientWin(
    Clock::duration networking_operation_timeout)
    : PlatformClientWin(
          networking_operation_timeout,
          std::make_unique<TaskRunnerImpl>(
              openscreen::Clock::now)) {  // Fix 2: Pass Clock::now
  task_runner_thread_ = std::make_optional<std::thread>(
      [this]() { task_runner_->RunUntilStopped(); });
}

PlatformClientWin::PlatformClientWin(
    Clock::duration networking_operation_timeout,
    std::unique_ptr<TaskRunnerImpl> task_runner)
    : task_runner_(std::move(task_runner)),
      networking_loop_timeout_(networking_operation_timeout) {
  OSP_DCHECK(task_runner_);

  networking_loop_thread_ =
      std::thread([this]() { RunNetworkLoopUntilStopped(); });
}

PlatformClientWin::~PlatformClientWin() {
  networking_loop_running_ = false;
  networking_loop_thread_.join();

  if (task_runner_thread_.has_value()) {
    task_runner_
        ->RequestStopSoon();  // Fix 1: Change Stop() to RequestStopSoon()
    task_runner_thread_->join();
  }

  udp_socket_reader_.reset();
  waiter_.reset();
  task_runner_.reset();
}

UdpSocketReaderWin* PlatformClientWin::udp_socket_reader() {
  std::call_once(udp_socket_reader_initialization_, [this]() {
    OSP_DCHECK(waiter_);  // waiter_ must be initialized first.
    udp_socket_reader_ = std::make_unique<UdpSocketReaderWin>(*waiter_);
  });
  return udp_socket_reader_.get();
}

TaskRunner& PlatformClientWin::GetTaskRunner() {
  return *task_runner_;
}

void PlatformClientWin::RunNetworkLoopUntilStopped() {
  waiter_ = std::make_unique<SocketHandleWaiterWin>(
      openscreen::Clock::now);  // Fix 3: Instantiate SocketHandleWaiterWin

  std::call_once(udp_socket_reader_initialization_, [this]() {
    OSP_DCHECK(waiter_);
    udp_socket_reader_ = std::make_unique<UdpSocketReaderWin>(*waiter_);
  });

  while (networking_loop_running_) {
    waiter_->ProcessHandles(networking_loop_timeout_);
  }
}

}  // namespace openscreen
