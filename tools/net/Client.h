/*
Copyright (c) 2018 - 2019 Adam Kaniewski

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
#include "SocketObject.h"

#include <mutex>

class Message;
class Client;
class Connection;

class ClientManager {
public:
  virtual void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) = 0;
  virtual void OnClientClosed(std::shared_ptr<Client> client) = 0;
  virtual void OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) = 0;
};

class Client : public SocketObject {

friend std::shared_ptr<Client> Connection::CreateClient(int, const std::string&);
friend void Connection::Accept(int);

public:
  void Start(std::weak_ptr<ClientManager> mgr, bool is_raw = false);
  void Send(std::shared_ptr<Message> msg);
  int GetId();

  const std::string& GetUrl();
  const std::string& GetIp();
  int GetPort();
  std::shared_ptr<Client> SharedPtr();

  void OnMsgWrite(std::shared_ptr<Message> msg, bool status) override;
  void OnDataRead(Data& data) override;
  bool IsActive() override;

protected:
  static uint32_t NextId();
  int _id;
  std::weak_ptr<ClientManager> _manager;
  bool _is_raw;
  std::atomic_bool _is_started;
  MessageBuilder _msg_builder;
  static std::atomic<uint32_t> _id_counter;
private:
  Client(size_t raw_handle,
         std::shared_ptr<Connection> connection,
         const std::string& ip = {},
         int port = -1,
         const std::string& url = {});
  std::string _url;
  std::string _ip;
  int _port;
  std::shared_ptr<Connection> _connection;
};
