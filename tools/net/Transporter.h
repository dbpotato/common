/*
Copyright (c) 2019 Adam Kaniewski

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

#include "Collector.h"

#include <memory>
#include <vector>
#include <sys/select.h>

class Connection;
class SocketObject;
class Message;

template<class T>
class LockablePtr {
private:
  std::weak_ptr<T> _weak;
  std::shared_ptr<T> _shared;
public:
  LockablePtr(std::weak_ptr<T> ptr) : _weak(ptr){}

  std::shared_ptr<T> Get() {
    return _shared;
  }

  std::shared_ptr<T> Lock() {
    _shared = _weak.lock();
    return _shared;
  }

  void Unlock() {
    _shared.reset();
  }
};

struct SendRequest {
  LockablePtr<SocketObject> _obj;
  std::shared_ptr<Message> _msg;
  SendRequest(std::weak_ptr<SocketObject> obj, std::shared_ptr<Message> msg)
      : _obj(obj)
      , _msg(msg){}
  void Unlock() {
    _obj.Unlock();
  }
};

class Transporter {
public:
  Transporter();
  void Init(std::shared_ptr<Connection> connection);
  void AddSendRequest(SendRequest req);
  bool RunOnce(std::vector<std::shared_ptr<SocketObject> >& objects); 

private:
  void UpdateSendRequests();
  int Select(fd_set& rfds, fd_set& wfds, std::vector<std::shared_ptr<SocketObject> >& objects);

  Collector<SendRequest> _collector;
  std::shared_ptr<Connection> _connection;
  std::vector<SendRequest> _req_vec;
};
