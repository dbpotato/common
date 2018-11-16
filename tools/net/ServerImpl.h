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

#include "Message.h"
#include "Client.h"
#include "Server.h"

#include <memory>
#include <map>
#include <vector>
#include <atomic>


class ServerListener : public std::enable_shared_from_this<ServerListener> {
public:
  virtual void OnClientConnected(std::shared_ptr<Client> client) = 0;
  virtual void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) = 0;
  virtual void OnClientClosed(std::shared_ptr<Client> client) = 0;
  virtual void OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) = 0;
};

class ServerImpl : public ServerManager,
                   public ClientManager,
                   public std::enable_shared_from_this<ServerImpl> {
public:
  ServerImpl();
  bool Init(int port, std::shared_ptr<Connection> connection, bool is_raw = false );
  void AddListener(std::weak_ptr<ServerListener>);
  std::shared_ptr<Client> GetClient(uint32_t id);
  void GetClients(std::vector<std::shared_ptr<Client> >& vec);

  //ServerManager interface implementation
  void OnClientConnected(std::shared_ptr<Client> client) override;

  //ClientManager interface implementations
  virtual void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  virtual void OnClientClosed(std::shared_ptr<Client> client) override;
  virtual void OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) override;

protected:
  void AddClient(std::shared_ptr<Client> client);
  bool RemoveClient(uint32_t id);
  void Clear();

  bool _is_raw;
  std::shared_ptr<Server> _server;
  std::shared_ptr<Connection> _connection;
  std::map<uint32_t, std::shared_ptr<Client> > _clients;
  std::vector<std::weak_ptr<ServerListener> > _listeners;
  std::mutex _mutex;
};