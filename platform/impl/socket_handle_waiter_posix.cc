// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/socket_handle_waiter_posix.h"

#include <time.h>
#include <unistd.h>  // For close()

#include <algorithm>
#include <cerrno>         // For errno
#include <chrono>         // For duration casts
#include <cstring>        // For strerror
#include <unordered_map>  // For kqueue fd to handle mapping
#include <vector>

#include "platform/base/error.h"
#include "platform/impl/socket_handle_posix.h"
#include "platform/impl/timeval_posix.h"
#include "util/osp_logging.h"

// Includes for kqueue on Apple systems.
#if BUILDFLAG(IS_APPLE)
#include <sys/event.h>
#include <sys/types.h>

// Default to using epoll on non-Apple systems.
#else
#include <sys/epoll.h>
#endif

namespace openscreen {

SocketHandleWaiterPosix::SocketHandleWaiterPosix(
    ClockNowFunctionPtr now_function)
    : SocketHandleWaiter(now_function), is_running_(false) {
#if BUILDFLAG(IS_APPLE)
  watcher_fd_ = kqueue();
#else
  watcher_fd_ = epoll_create1(EPOLL_CLOEXEC);
#endif
  if (watcher_fd_ == -1) {
    OSP_LOG_FATAL << "failed to subscribe to events: " << strerror(errno);
  }
}

SocketHandleWaiterPosix::~SocketHandleWaiterPosix() {
  if (watcher_fd_ != -1) {
    close(watcher_fd_);
    watcher_fd_ = -1;
  }
}

ErrorOr<std::vector<SocketHandleWaiterPosix::ReadyHandle>>
SocketHandleWaiterPosix::AwaitSocketsReadable(
    const std::vector<SocketHandleRef>& socket_handles,
    const Clock::duration& timeout) {
  OSP_CHECK(!socket_handles.empty());

  if (watcher_fd_ == -1) {
    OSP_LOG_ERROR << "watcher fd is not initialized.";
    return Error::Code::kInitializationFailure;
  }

#if BUILDFLAG(IS_APPLE)
  std::vector<struct kevent> changelist;
  std::unordered_set<int> current_fds_set;
  std::unordered_map<int, SocketHandleRef> fd_to_handle_ref_map;

  for (const auto& sh_ref : socket_handles) {
    if (sh_ref.get().fd >= 0) {  // Only watch valid FDs
      current_fds_set.insert(sh_ref.get().fd);
      fd_to_handle_ref_map.emplace(sh_ref.get().fd, sh_ref);
    }
  }

  for (auto it = watched_fds_.begin(); it != watched_fds_.end();) {
    if (current_fds_set.find(*it) == current_fds_set.end()) {
      struct kevent kev_read_del, kev_write_del;
      EV_SET(&kev_read_del, *it, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
      changelist.push_back(kev_read_del);
      EV_SET(&kev_write_del, *it, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
      changelist.push_back(kev_write_del);
      it = watched_fds_.erase(it);
    } else {
      ++it;
    }
  }

  for (int fd_to_add : current_fds_set) {
    if (watched_fds_.find(fd_to_add) == watched_fds_.end()) {
      struct kevent kev_read_add, kev_write_add;
      // Store fd in udata for retrieval, ensures correct mapping if handle
      // object changes
      EV_SET(&kev_read_add, fd_to_add, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
             reinterpret_cast<void*>(static_cast<intptr_t>(fd_to_add)));
      changelist.push_back(kev_read_add);
      EV_SET(&kev_write_add, fd_to_add, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0,
             reinterpret_cast<void*>(static_cast<intptr_t>(fd_to_add)));
      changelist.push_back(kev_write_add);
      watched_fds_.insert(fd_to_add);
    }
  }

  if (watched_fds_.empty() && current_fds_set.empty()) {
    struct timeval tv_sleep = ToTimeval(timeout);
    select(0, nullptr, nullptr, nullptr, &tv_sleep);
    return Error::Code::kAgain;
  }

  std::vector<struct kevent> events_received(watched_fds_.size() * 2);
  struct timespec timeout_ts = ToTimespec(timeout);

  int num_events =
      kevent(watcher_fd_, changelist.data(), changelist.size(),
             events_received.data(), events_received.size(), &timeout_ts);

  if (num_events == -1) {
    if (errno == EINTR) {
      return Error::Code::kAgain;
    }
    OSP_LOG_ERROR << "kevent failed: " << strerror(errno);
    return Error::Code::kIOFailure;
  }
  if (num_events == 0) {
    return Error::Code::kAgain;
  }

  std::vector<ReadyHandle> changed_handles;
  std::unordered_map<int, uint32_t> fd_event_flags;

  for (int i = 0; i < num_events; ++i) {
    const struct kevent& ev = events_received[i];
    // ev.ident is the fd for EVFILT_READ/WRITE
    int ready_fd = static_cast<int>(ev.ident);

    if (ev.flags & EV_ERROR) {
      OSP_LOG_WARN << "kevent error for fd " << ready_fd << " (errno "
                   << ev.data << "): " << strerror(ev.data);
      watched_fds_.erase(ready_fd);  // Remove from our tracking
      // Attempt to delete from kqueue explicitly, though EV_ERROR might mean
      // it's already gone.
      struct kevent del_ev_read, del_ev_write;
      EV_SET(&del_ev_read, ready_fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
      EV_SET(&del_ev_write, ready_fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
      struct kevent temp_cl[] = {del_ev_read, del_ev_write};
      kevent(watcher_fd_, temp_cl, 2, nullptr, 0, nullptr);  // Best effort
      continue;
    }

    if (ev.filter == EVFILT_READ) {
      fd_event_flags[ready_fd] |= Flags::kReadable;
    } else if (ev.filter == EVFILT_WRITE) {
      fd_event_flags[ready_fd] |= Flags::kWriteable;
    }

    if (ev.flags & EV_EOF) {
      // EOF implies readability (to detect the EOF) and often writability
      // (though writes may fail). This ensures the application layer can
      // discover the socket state.
      fd_event_flags[ready_fd] |= (Flags::kReadable | Flags::kWriteable);
      OSP_DVLOG << "kqueue EV_EOF on fd " << ready_fd << " flags: " << ev.flags
                << " filter: " << ev.filter;
    }
  }

  changed_handles.reserve(fd_event_flags.size());
  for (const auto& pair : fd_event_flags) {
    int ready_fd = pair.first;
    uint32_t flags = pair.second;
    if (flags == 0)
      continue;

    auto it = fd_to_handle_ref_map.find(ready_fd);
    if (it != fd_to_handle_ref_map.end()) {
      changed_handles.push_back({it->second, flags});
    } else {
      OSP_LOG_WARN << "kevent processed fd not in initial map: " << ready_fd
                   << ". It might have been closed and removed concurrently.";
    }
  }
  return changed_handles;

#else
  std::unordered_set<int> current_fds_set;
  for (const auto& sh_ref : socket_handles) {
    if (sh_ref.get().fd >= 0) {  // Only watch valid FDs
      current_fds_set.insert(sh_ref.get().fd);
    }
  }

  // Remove FDs no longer watched
  for (auto it = watched_fds_.begin(); it != watched_fds_.end();) {
    if (current_fds_set.find(*it) == current_fds_set.end()) {
      if (epoll_ctl(watcher_fd_, EPOLL_CTL_DEL, *it, nullptr) == -1) {
        // ENOENT is fine if FD was already closed and removed by kernel
        if (errno != ENOENT) {
          OSP_LOG_WARN << "epoll_ctl(DEL) failed for fd " << *it << ": "
                       << strerror(errno);
        }
      }
      it = watched_fds_.erase(it);
    } else {
      ++it;
    }
  }

  // Add new FDs
  for (int fd_to_add : current_fds_set) {
    if (watched_fds_.find(fd_to_add) == watched_fds_.end()) {
      struct epoll_event event;
      event.events = EPOLLIN | EPOLLOUT;  // Level-triggered for read and write
      event.data.fd = fd_to_add;
      if (epoll_ctl(watcher_fd_, EPOLL_CTL_ADD, fd_to_add, &event) == -1) {
        OSP_LOG_ERROR << "epoll_ctl(ADD) failed for fd " << fd_to_add << ": "
                      << strerror(errno);
      } else {
        watched_fds_.insert(fd_to_add);
      }
    }
  }

  if (watched_fds_.empty() && current_fds_set.empty()) {
    // epoll_wait with an empty set and timeout > 0 can be problematic or
    // undefined. Behave like select with no FDs.
    struct timeval tv_sleep = ToTimeval(timeout);
    select(0, nullptr, nullptr, nullptr, &tv_sleep);
    return Error::Code::kAgain;
  }

  std::vector<struct epoll_event> events_received(watched_fds_.size());
  int timeout_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count();
  if (timeout_ms < 0)
    timeout_ms = 0;

  int num_events = epoll_wait(watcher_fd_, events_received.data(),
                              events_received.size(), timeout_ms);

  if (num_events == -1) {
    if (errno == EINTR) {
      return Error::Code::kAgain;
    }
    OSP_LOG_ERROR << "epoll_wait failed: " << strerror(errno);
    return Error::Code::kIOFailure;
  }
  if (num_events == 0) {
    return Error::Code::kAgain;
  }

  std::vector<ReadyHandle> changed_handles;
  changed_handles.reserve(num_events);
  for (int i = 0; i < num_events; ++i) {
    int ready_fd = events_received[i].data.fd;
    uint32_t flags = 0;
    // Check for errors or hangup first, as these imply readability/writability
    // for error detection
    if (events_received[i].events & (EPOLLHUP | EPOLLERR)) {
      flags |= (Flags::kReadable | Flags::kWriteable);
      OSP_DVLOG << "epoll event EPOLLHUP/EPOLLERR ("
                << events_received[i].events << ") on fd " << ready_fd;
    }
    if (events_received[i].events & EPOLLIN) {
      flags |= Flags::kReadable;
    }
    if (events_received[i].events & EPOLLOUT) {
      flags |= Flags::kWriteable;
    }

    if (flags == 0)
      continue;

    auto it = std::find_if(socket_handles.begin(), socket_handles.end(),
                           [ready_fd](const SocketHandleRef& shr) {
                             return shr.get().fd == ready_fd;
                           });
    if (it != socket_handles.end()) {
      changed_handles.push_back({*it, flags});
    } else {
      OSP_LOG_WARN << "epoll_wait returned untracked fd: " << ready_fd
                   << ". It might have been closed and removed from watch list "
                      "concurrently.";
    }
  }
  return changed_handles;
#endif
}

void SocketHandleWaiterPosix::RunUntilStopped() {
  const bool was_running = is_running_.exchange(true);
  OSP_CHECK(!was_running);

  constexpr Clock::duration kHandleReadyTimeout = std::chrono::milliseconds(50);
  while (is_running_.load(std::memory_order_relaxed)) {
    const Error result = ProcessHandles(kHandleReadyTimeout);
    // kAgain is normal (timeout or no events). Other errors might be serious.
    if (!result.ok() && result.code() != Error::Code::kAgain) {
      OSP_LOG_ERROR << __func__ << "ProcessHandles() had an error: " << result;
      // TODO(jophba): add error handling?
      // Depending on the error, might want to stop or take other action.
      // For now, we just log and continue the loop if is_running_ is still
      // true.
    }
  }
}

void SocketHandleWaiterPosix::RequestStopSoon() {
  is_running_.store(false, std::memory_order_release);
}

}  // namespace openscreen
