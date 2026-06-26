/*
Copyright (c) 2020 - 2026 Adam Kaniewski

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
#include "ThreadLoop.h"

#include <cstring>
#include <set>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <array>

constexpr int EPOOL_MAX_EVENTS = 32;

std::weak_ptr<Epool> Epool::_instance;

FdListenerInfo::FdListenerInfo(std::weak_ptr<FdListener> object)
    : _object(object)
    , _event_flags(0) {
}

std::weak_ptr<FdListener> FdListenerInfo::GetObject() {
  return _object;
}

int FdListenerInfo::GetEventFlags() {
  return _event_flags;
}

void FdListenerInfo::SetEventFlags(int flags) {
  _event_flags = flags;
}

Epool::Epool()
    : _epool_fd(-1)
    , _wake_up_fd(-1) {
}

Epool::~Epool() {
  if(_epool_fd != -1) {
    close(_epool_fd);
  }
  if(_wake_up_fd != -1) {
    close(_wake_up_fd);
  }
}

std::shared_ptr<Epool> Epool::GetInstance() {
  std::shared_ptr<Epool> instance;
  if(!(instance =_instance.lock())) {
    instance.reset(new Epool());
    if(instance->Init()) {
      _instance = instance;
    } else {
      return nullptr;
    }
  }
  return instance;
}

bool Epool::Init() {
  _epool_fd = epoll_create1(0);

  if (_epool_fd == -1) {
    DLOG(error, "epoll_create failed");
    return false;
  }

  if(!CreateWakeFd()) {
    return false;
  }

  _thread_loop = std::make_shared<ThreadLoop>();
  _thread_loop->Init();
  WaitForEvents();
  return true;
}

bool Epool::CreateWakeFd() {
  _wake_up_fd = eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
  if(_wake_up_fd == -1) {
    DLOG(error, "eventfd() failed");
    return false;
  }

  struct epoll_event event;
  std::memset(&event, 0 , sizeof(epoll_event));
  event.data.fd = _wake_up_fd;
  event.events = EPOLLIN;

  if (epoll_ctl(_epool_fd, EPOLL_CTL_ADD, _wake_up_fd, &event) == -1) {
    DLOG(error, "Failed to add wake_fd");
    return false;
  }
  return true;
}

void Epool::Wake() {
  uint64_t value = 1;
  if(write(_wake_up_fd, &value, sizeof(value)) != sizeof(value)) {
    DLOG(error, "Wake failed");
  }
}

void Epool::ClearWake() {
  uint64_t value = 0;
  if(read(_wake_up_fd, &value, sizeof(value)) <=0 ){
    DLOG(error, "ClearWake failed");
  }
}

void Epool::AddListener(std::shared_ptr<FdListener> obj, bool wait_for_read) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Epool::AddListener, shared_from_this(), obj, wait_for_read));
    Wake();
    return;
  }

  int fd = obj->GetFd();

  auto listener = GetFdListener(fd);
  if(listener){
    DLOG(error, "AddListener FAILED - Already exists : {}", fd);
    NotifyListenerOnError(obj, false);
    return;
  } else {
    _listeners.insert(std::make_pair<int, FdListenerInfo>(std::move(obj->GetFd()), FdListenerInfo(obj)));
  }

  struct epoll_event event;
  std::memset(&event, 0 , sizeof(epoll_event));
  event.data.fd = fd;

  if (epoll_ctl(_epool_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
    DLOG(error, "AddListener FAILED - EPOLL_CTL_ADD : {}", fd);
    NotifyListenerOnError(obj, true);
    _listeners.erase(fd);
    return;
  }

  if(wait_for_read) {
    SetObservedEvent(fd, EPOLLIN, true);
  }
}

void Epool::RemoveListener(int fd) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Epool::RemoveListener, shared_from_this(), fd));
    Wake();
    return;
  }

  _listeners.erase(fd);
  epoll_ctl(_epool_fd, EPOLL_CTL_DEL, fd, nullptr);
}

void Epool::SetListenerAwaitingRead(std::shared_ptr<FdListener> obj, bool waiting_for_read) {
  SetObservedEvent(obj->GetFd(), EPOLLIN, waiting_for_read);
}

void Epool::SetListenerAwaitingWrite(std::shared_ptr<FdListener> obj, bool waiting_for_write) {
  SetObservedEvent(obj->GetFd(), EPOLLOUT, waiting_for_write);
}

void Epool::SetListenerAwaitingFlags(std::shared_ptr<FdListener> obj, bool waiting_for_read, bool waiting_for_write) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Epool::SetListenerAwaitingFlags, shared_from_this(), obj, waiting_for_read, waiting_for_write));
    Wake();
    return;
  }
  SetListenerAwaitingRead(obj, waiting_for_read);
  SetListenerAwaitingWrite(obj, waiting_for_write);
}

void Epool::SetObservedEvent(int fd, uint32_t event_flag, bool enabled) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Epool::SetObservedEvent, shared_from_this(), fd, event_flag, enabled));
    Wake();
    return;
  }

  auto listener_it = _listeners.find(fd);
  if(listener_it == _listeners.end()) {
    DLOG(warn, "SetObservedEvent - listener not found : {}", fd);
    return;
  }

  int current_flags = listener_it->second.GetEventFlags();
  struct epoll_event event;
  std::memset(&event, 0 , sizeof(epoll_event));
  event.data.fd = fd;
  if(enabled) {
    event.events = current_flags | event_flag;
  } else {
    event.events = current_flags & ~event_flag;
  }
  listener_it->second.SetEventFlags(event.events);

  if (epoll_ctl(_epool_fd, EPOLL_CTL_MOD, fd, &event) == -1) {
    DLOG(error, "EPOLL_CTL_MOD failed : {} : {} : {}", fd, current_flags, (uint32_t)event.events);
    NotifyListenerOnError(fd, true);
    _listeners.erase(fd);
    return;
  }
}

void Epool::WaitForEvents() {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Epool::WaitForEvents, shared_from_this()));
    return;
  }

  std::array<struct epoll_event, EPOOL_MAX_EVENTS> events;
  int epool_size = epoll_wait(_epool_fd, events.data(), EPOOL_MAX_EVENTS, -1);
  std::set<int> ignored_fds;

  for (int i = 0; i < epool_size; ++i) {
    int fd = (int)events[i].data.fd;
    uint32_t event = events[i].events;

    if(fd == _wake_up_fd) {
      ClearWake();
      continue;
    }

    if(ignored_fds.find(fd) != ignored_fds.end()) {
      continue;
    }

    if(event == EPOLLHUP) {
      RemoveListener(fd);
      continue;
    }

    auto listener = GetFdListener(fd);
    if(!listener) {
      ignored_fds.insert(fd);
      continue;
    }

    SetObservedEvent(fd, event, false);
    HandleFdEvent(listener, fd, event);
  }
  _thread_loop->Post(std::bind(&Epool::WaitForEvents, shared_from_this()));
}

void Epool::HandleFdEvent(std::shared_ptr<FdListener> listener, int fd, uint32_t event) {
  if(event & EPOLLIN) {
    listener->OnFdReadReady();
  }

  if(event & EPOLLOUT) {
    listener->OnFdWriteReady();
  }

  if(event & EPOLLERR) {
    listener->OnFdOperationError(true);
  }
}

std::shared_ptr<FdListener> Epool::GetFdListener(int fd) {
  std::shared_ptr<FdListener> result = nullptr;
  auto listener_it =_listeners.find(fd);
  if(listener_it != _listeners.end()) {
    auto wrapper = listener_it->second;
    result = wrapper.GetObject().lock();
  }

  if(!result && (listener_it != _listeners.end())) {
    RemoveListener(fd);
  }

  return result;
}

void Epool::NotifyListenerOnError(int fd, bool is_epool_err) {
  auto listener = GetFdListener(fd);
  if(listener) {
    NotifyListenerOnError(listener, is_epool_err);
  }
}

void Epool::NotifyListenerOnError(std::shared_ptr<FdListener> obj, bool is_epool_err) {
  obj->OnFdOperationError(is_epool_err);
}