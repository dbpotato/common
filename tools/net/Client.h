/*
Copyright (c) 2018 Adam Kaniewski

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
#include "MessageBuilder.h"

#include <mutex>

class Message;
class Client;

class ClientManager {
public:
  virtual void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) = 0;
  virtual void OnClientClosed(std::shared_ptr<Client> client) = 0;
  virtual void OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) = 0;
};

class Client : public std::enable_shared_from_this<Client> {

friend std::shared_ptr<Client> Connection::CreateClient(int, const std::string&);
friend std::shared_ptr<Client> Connection::Accept(int);

public:
  ~Client();
  void Start(std::weak_ptr<ClientManager> mgr, bool is_raw = false);
  void OnClosed();
  void Send(std::shared_ptr<Message> msg);
  int GetId();
  size_t MsgQueueSize();
  void OnRead(Data& data);
  void OnWrite(std::shared_ptr<Message> msg, bool status);
  std::shared_ptr<Message> RemoveMsg();
  std::atomic_bool& IsStarted();

protected:
  static uint32_t NextId();

  int _id;
  std::weak_ptr<ClientManager> _manager;
  std::mutex _mutex;
  std::vector<std::shared_ptr<Message> > _msg_query;
  bool _is_raw;
  std::atomic_bool _is_started;
  MessageBuilder _msg_builder;
  static std::atomic<uint32_t> _id_counter;
private:
  Client();
};
