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
#include "PosixThread.h"
#include "Utils.h"

#include <memory>
#include <mutex>
#include <vector>
#include <sys/select.h>

class Connection;
class Client;
class Message;
class SocketObject;


struct SendRequest {
  LockablePtr<Client> _obj;
  std::shared_ptr<Message> _msg;
  SendRequest(std::weak_ptr<Client> obj, std::shared_ptr<Message> msg)
      : _obj(obj)
      , _msg(msg){}
};

class Transporter : public std::enable_shared_from_this<Transporter>
                  , public ThreadObject {
public:
  static std::shared_ptr<Transporter> GetTransporter(std::weak_ptr<Connection>);
  ~Transporter();
  void AddSendRequest(SendRequest req);
  bool RunOnce(std::vector<std::shared_ptr<SocketObject> >& objects); 
  void OnThreadStarted(int thread_id) override;

private:
  Transporter();
  void Init();
  void UpdateSendRequests();
  void CollectActiveSockets();
  int Select(fd_set& rfds, fd_set& wfds, std::vector<std::shared_ptr<SocketObject> >& objects);
  int HandleReceive(std::vector<std::shared_ptr<SocketObject> >& objects, fd_set& rfds);
  bool HandleSend(fd_set& wfds);

  void AddConnection(std::weak_ptr<Connection>);

  Collector<SendRequest> _collector;
  std::vector<std::weak_ptr<Connection> > _connections;
  std::vector<SendRequest> _req_vec;
  static std::weak_ptr<Transporter> _instance;
  PosixThread _run_thread;
  std::mutex _conn_mutex;
};
