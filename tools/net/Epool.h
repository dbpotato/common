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

#pragma once

#include "PosixThread.h"

#include <memory>
#include <vector>
#include <condition_variable>

class SocketObject;


class SocketEventListener {
public :
  class Event {
  public:
    Event(uint32_t flags);
    bool CanRead();
    bool CanWrite();
    bool Closed();
  private:
    uint32_t _flags;
  };

  virtual void OnSocketEvents(std::vector<std::pair<int, Event>>& events) = 0;
};

class Epool : public std::enable_shared_from_this<Epool>
            , public ThreadObject {
public:
  Epool();
  bool Init(std::weak_ptr<SocketEventListener> listener);
  void OnThreadStarted(int thread_id) override;

  bool AddSocket(int socket_fd);
  void RemoveSocket(int socket_fd);
  void SetFlags(int socket_fd, bool can_read, bool can_write);
  //bool SameThread();
  void NotifyEventsHandled();

private:
  int _epool_fd;
  bool _events_handed;
  PosixThread _run_thread;
  std::condition_variable _condition;
  std::mutex _condition_mutex;
  std::weak_ptr<SocketEventListener> _listener;
};
