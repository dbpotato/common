/*
Copyright (c) 2018 - 2023 Adam Kaniewski

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

#include "Client.h"
#include "NetUtils.h"
#include "SocketObject.h"

#include <map>
#include <memory>
#include <vector>


class Message;
class SocketContext;

class Server : public ClientManager,
               public SocketObject {

friend  std::shared_ptr<Server> Connection::CreateServer(int,
                                                         std::vector<std::weak_ptr<ClientManager>>&,
                                                         std::shared_ptr<SocketContext>);

public:
  std::shared_ptr<Client> GetClient(uint32_t id);
  void GetClients(std::vector<std::shared_ptr<Client> >& vec);
  bool RemoveClient(std::shared_ptr<Client> client);
  bool RemoveClient(uint32_t id);

  //ClientManager interface implementations
  virtual void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  virtual void OnClientClosed(std::shared_ptr<Client> client) override;
  virtual void OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) override;
  virtual bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  virtual void OnClientConnected(std::shared_ptr<Client> client) override;

protected:
  void AddClient(std::shared_ptr<Client> client);
  void Clear();

  std::shared_ptr<Server> _server;
  std::shared_ptr<Connection> _connection;
  std::map<uint32_t, std::shared_ptr<Client> > _clients;
  std::vector<std::weak_ptr<ClientManager> > _listeners;
  std::mutex _mutex;

private :
  Server(int socket_fd,
         std::shared_ptr<Connection> connection,
         std::shared_ptr<SocketContext> context,
         std::vector<std::weak_ptr<ClientManager> >& listeners);
  void Init();
};
