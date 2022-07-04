/*
Copyright (c) 2018 - 2021 Adam Kaniewski

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
#include "Utils.h"

#include <mutex>


class Message;
class Client;
class Server;

class ClientManager {
public:
  virtual void OnServerCreated(std::weak_ptr<Server> server);
  virtual void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg);
  virtual bool OnClientConnected(std::shared_ptr<Client> client, NetError err);
  virtual void OnClientClosed(std::shared_ptr<Client> client);
  virtual void OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success);
  virtual bool IsRaw() = 0;
};

class Client : public SocketObject {

friend void Connection::CreateClient(int, const std::string&, std::weak_ptr<ClientManager>);
friend void Connection::Accept(std::shared_ptr<SocketObject>);

public:
  void Send(std::shared_ptr<Message> msg);

  void OnMsgWrite(std::shared_ptr<Message> msg, bool status);
  void OnDataRead(Data& data);
  void OnConnectionClosed() override;
  bool OnConnected(NetError err);

  uint32_t GetId();
  const std::string& GetUrl();
  const std::string& GetIp();
  int GetPort();
  std::shared_ptr<Client> SharedPtr();
  void Update(int socket, const std::string& ip);
  void SetManager(std::weak_ptr<ClientManager> manager);
  void SetMsgBuilder(std::unique_ptr<MessageBuilder> msg_builder);

protected:
  static uint32_t NextId();
  static std::atomic<uint32_t> _id_counter;
private:
  Client(int socket_fd,
         std::shared_ptr<Connection> connection,
         const std::string& ip = {},
         int port = DEFAULT_SOCKET,
         const std::string& url = {},
         std::weak_ptr<ClientManager> manager = {});

  std::string _ip;
  int _port;
  std::string _url;
  std::weak_ptr<ClientManager> _manager;
  uint32_t _id;
  std::unique_ptr<MessageBuilder> _msg_builder;
};
