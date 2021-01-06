/*
Copyright (c) 2020 Adam Kaniewski

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Epool.h"
#include "Logger.h"

#include <cstring>
#include <sys/epoll.h>
#include <unistd.h>
#include <array>


constexpr int EPOOL_MAX_EVENTS = 32;

SocketEventListener::Event::Event(uint32_t flags) : _flags(flags) {
}

bool SocketEventListener::Event::CanRead() {
  return _flags & EPOLLIN;
}

bool SocketEventListener::Event::CanWrite() {
  return _flags & EPOLLOUT;
}

bool SocketEventListener::Event::Closed() {
  return((_flags & EPOLLERR) ||
         (_flags & EPOLLHUP));
}


Epool::Epool()
    : _epool_fd(-1)
    , _events_handed(true) {
}

bool Epool::Init(std::weak_ptr<SocketEventListener> listener) {
  _listener = listener;
  _epool_fd = epoll_create(1);

  if (_epool_fd != -1) {
    _run_thread.Run(shared_from_this(), 0);
    return true;
  }

  return false;
}

void Epool::NotifyEventsHandled() {
  std::unique_lock<std::mutex> lock(_condition_mutex);
  _events_handed = true;
  _condition.notify_one();
}

bool Epool::AddSocket(int socket_fd) {
  struct epoll_event event;
  std::memset(&event, 0 , sizeof(epoll_event));
  event.data.fd = socket_fd;

  if (epoll_ctl(_epool_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
    return false;
  }
  return true;
}

void Epool::RemoveSocket(int socket_fd) {
  if (epoll_ctl(_epool_fd, EPOLL_CTL_DEL, socket_fd, nullptr) == -1) {
    DLOG(warn, "Epool::RemoveSocket : {} failed", socket_fd);
  }
}

void Epool::SetFlags(int socket_fd, bool can_read, bool can_write) {
  struct epoll_event event;
  std::memset(&event, 0 , sizeof(epoll_event));
  event.data.fd = socket_fd;
  if(can_read) {
    event.events = EPOLLIN;
  }
  if(can_write) {
    event.events = EPOLLIN | EPOLLOUT;
  }
  if (epoll_ctl(_epool_fd, EPOLL_CTL_MOD, socket_fd, &event) == -1) {
    DLOG(error, "EPOLL_CTL_MOD : {} failed", socket_fd);
  }
}

void Epool::OnThreadStarted(int thread_id) {
  while(_run_thread.ShouldRun()) {
    {
      std::unique_lock<std::mutex> lock(_condition_mutex);
      _condition.wait(lock, [this]{return _events_handed || !_run_thread.ShouldRun();});
      _events_handed = false;
    }

    if( !_run_thread.ShouldRun())
      return;

    std::array<struct epoll_event, EPOOL_MAX_EVENTS> events;
    int epool_size = epoll_wait(_epool_fd, events.data(), EPOOL_MAX_EVENTS, -1);
    if(auto listener = _listener.lock()) {
      std::vector<std::pair<int, SocketEventListener::Event>> events_vec;
      for (int i = 0; i < epool_size; ++i) {
        int socket_fd = (int)events[i].data.fd;
        SocketEventListener::Event soc_event(events[i].events);
        events_vec.emplace_back(std::make_pair(socket_fd, soc_event));
      }
      listener->OnSocketEvents(events_vec);
    }
    else {
     return;
    }
  }
}
