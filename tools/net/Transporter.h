/*
Copyright (c) 2019 - 2022 Adam Kaniewski

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

#include "Utils.h"
#include "Epool.h"
#include "Message.h"

#include <memory>
#include <vector>
#include <map>
#include <sys/select.h>

class Connection;
class Client;
class Message;
class MessageWriteRequest;
class SocketObject;
class ThreadLoop;
class Epool;


class SocketInfo {
public:
  SocketInfo(std::weak_ptr<SocketObject> object);
  std::shared_ptr<SocketObject> lock();

  std::weak_ptr<SocketObject> _object;
  bool _pending_read;
  bool _pending_write;
  bool _active;
  bool _added_to_epool;
};

class Transporter : public std::enable_shared_from_this<Transporter>
                  , public SocketEventListener {
public:
  static std::shared_ptr<Transporter> GetInstance();
  void AddSendRequest(int socket_fd, MessageWriteRequest req);
  void AddSocket(std::shared_ptr<SocketObject> socket_obj);
  void RemoveSocket(int socket_fd);
  void EnableSocket(std::shared_ptr<SocketObject> socket_obj);
  void DisableSocket(std::shared_ptr<SocketObject> socket_obj);
  void OnSocketEvents(std::vector<std::pair<int, SocketEventListener::Event>>& events) override;
private:
  Transporter();
  void Init();
  void SendPending(std::shared_ptr<Client> client);

  std::map<int, std::vector<MessageWriteRequest>> _write_reqs;
  static std::weak_ptr<Transporter> _instance;
  std::map<int, SocketInfo> _sockets;
  std::shared_ptr<Epool> _epool;
  std::shared_ptr<ThreadLoop> _thread_loop;
};
