/*
Copyright (c) 2020 - 2023 Adam Kaniewski

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
#include "Connection.h"
#include "Logger.h"
#include "SocketObject.h"
#include "ThreadLoop.h"

#include <cstring>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <array>


constexpr int EPOOL_MAX_EVENTS = 32;

std::weak_ptr<Epool> Epool::_instance;

SocketInfo::SocketInfo(std::weak_ptr<SocketObject> object)
    : _object(object)
    , _event_flags(0) {
}

std::shared_ptr<SocketObject> SocketInfo::lock() {
  return _object.lock();
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
    DLOG(error, "Epool::Init : epoll_create failed");
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
    DLOG(error, "Epool : eventfd() failed");
    return false;
  }

  struct epoll_event event;
  std::memset(&event, 0 , sizeof(epoll_event));
  event.data.fd = _wake_up_fd;
  event.events = EPOLLIN;

  if (epoll_ctl(_epool_fd, EPOLL_CTL_ADD, _wake_up_fd, &event) == -1) {
    DLOG(error, "Epoll : Failed to add wake_fd");
    return false;
  }
  return true;
}

void Epool::Wake() {
  uint64_t value = 1;
  if(write(_wake_up_fd, &value, sizeof(value)) != sizeof(value)) {
    DLOG(error, "Epool::Wake failed");
  }
}

void Epool::ClearWake() {
  uint64_t value = 0;
  if(read(_wake_up_fd, &value, sizeof(value)) <=0 ){
    DLOG(error, "Epool::ClearWake failed");
  }
}

void Epool::AddSocket(std::shared_ptr<SocketObject> obj) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Epool::AddSocket, shared_from_this(), obj));
    Wake();
    return;
  }

  int socket_fd = obj->SocketFd();
  auto result = _sockets.insert(std::make_pair<int,SocketInfo>((int)socket_fd, SocketInfo(obj)));
  if(!result.second) {
    DLOG(error, "Epool::AddSocket FAILED - Already exists : {}", socket_fd);
    return;
  }
  struct epoll_event event;
  std::memset(&event, 0 , sizeof(epoll_event));
  event.data.fd = socket_fd;

  if (epoll_ctl(_epool_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
    DLOG(error, "Epool::AddSocket FAILED - EPOLL_CTL_ADD : {}", socket_fd);
  }
}

void Epool::RemoveSocket(int socket_fd) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Epool::RemoveSocket, shared_from_this(), socket_fd));
    Wake();
    return;
  }

  _sockets.erase(socket_fd);
  epoll_ctl(_epool_fd, EPOLL_CTL_DEL, socket_fd, nullptr);
  close(socket_fd);
}

void Epool::SetSocketAwaitingRead(std::shared_ptr<SocketObject> obj, bool waiting_for_read) {
  SetObservedEvent(obj->SocketFd(), EPOLLIN, waiting_for_read);
}

void Epool::SetSocketAwaitingWrite(std::shared_ptr<SocketObject> obj, bool waiting_for_write) {
  SetObservedEvent(obj->SocketFd(), EPOLLOUT, waiting_for_write);
}

void Epool::SetSocketAwaitingFlags(std::shared_ptr<SocketObject> obj, bool waiting_for_read, bool waiting_for_write) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Epool::SetSocketAwaitingFlags, shared_from_this(), obj, waiting_for_read, waiting_for_write));
    Wake();
    return;
  }
  SetSocketAwaitingRead(obj, waiting_for_read);
  SetSocketAwaitingWrite(obj, waiting_for_write);
}

void Epool::SetObservedEvent(int socket_fd, int event_flag, bool enabled) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Epool::SetObservedEvent, shared_from_this(), socket_fd, event_flag, enabled));
    Wake();
    return;
  }

  auto socket_it =_sockets.find(socket_fd);
  if(socket_it == _sockets.end()) {
    DLOG(warn, "Epool::HandleSocketEvent - SocketObject not found : {}", socket_fd);
    return;
  }

  int current_flags = socket_it->second._event_flags;
  struct epoll_event event;
  std::memset(&event, 0 , sizeof(epoll_event));
  event.data.fd = socket_fd;
  if(enabled) {
    event.events = current_flags | event_flag;
  } else {
    event.events = current_flags & ~event_flag;
  }
  socket_it->second._event_flags = event.events;

  if (epoll_ctl(_epool_fd, EPOLL_CTL_MOD, socket_fd, &event) == -1) {
    DLOG(error, "Epool : EPOLL_CTL_MOD failed : {} : {} : {}", socket_fd, current_flags, event.events);
  }
}

void Epool::WaitForEvents() {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Epool::WaitForEvents, shared_from_this()));
    return;
  }

  std::array<struct epoll_event, EPOOL_MAX_EVENTS> events;
  int epool_size = epoll_wait(_epool_fd, events.data(), EPOOL_MAX_EVENTS, -1);

  for (int i = 0; i < epool_size; ++i) {
    int socket_fd = (int)events[i].data.fd;
    if(socket_fd == _wake_up_fd) {
      ClearWake();
      continue;
    }
    int event = events[i].events;
    if(!(event & EPOLLIN) && !(event & EPOLLOUT)) {
      continue;
    }
    SetObservedEvent(socket_fd, event, false);
    HandleSocketEvent(socket_fd, event);
  }
  _thread_loop->Post(std::bind(&Epool::WaitForEvents, shared_from_this()));
}

void Epool::HandleSocketEvent(int socket_fd, int event) {
  auto socket_it =_sockets.find(socket_fd);
  if(socket_it == _sockets.end()) {
    DLOG(warn, "Epool::HandleSocketEvent - SocketObject not found : {}", socket_fd);
    return;
  }

  auto wrapper = socket_it->second;
  auto socket_obj = wrapper.lock();
  if(!socket_obj) {
    DLOG(warn, "Epool::HandleSocketEvent  - SocketObject already released : {}", socket_fd);
    RemoveSocket(socket_fd);
    return;
  }

  if(event & EPOLLIN) {
    socket_obj->GetConnection()->OnSocketReadReady(socket_obj);
  }

  if(event & EPOLLOUT) {
    socket_obj->GetConnection()->OnSocketWriteReady(socket_obj);
  }
}
