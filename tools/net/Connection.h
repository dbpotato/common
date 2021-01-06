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

#include "SocketObject.h"
#include "Utils.h"

#include <string>
#include <vector>
#include <mutex>


class Message;
class Client;
class ClientManager;
class ClientOwner;
class Server;
class SocketContext;
class Transporter;
class ConnectThread;


class Connection : public std::enable_shared_from_this<Connection> {

friend class Transporter;
friend SocketObject::~SocketObject();

public: //TODO
  static std::shared_ptr<Connection> CreateBasic();
  virtual ~Connection();
  void CreateClient(int port,
                    const std::string& host,
                    std::weak_ptr<ClientManager> mgr);

  std::shared_ptr<Server> CreateServer(int port,
                                       std::vector<std::weak_ptr<ClientManager> >& listeners,
                                       bool is_raw);

  std::shared_ptr<Server> CreateServer(int port,
                                       std::shared_ptr<ClientManager> listener);

  void SendMsg(std::shared_ptr<Client>, std::shared_ptr<Message> msg);
  void Accept(std::shared_ptr<SocketObject> obj);
  void NotifySocketActiveChanged(std::shared_ptr<SocketObject> obj);

  void GetActiveSockets(std::vector<std::shared_ptr<SocketObject> >& out_objects);

  virtual NetError AfterSocketCreated(std::shared_ptr<SocketObject> obj);
  virtual NetError AfterSocketAccepted(std::shared_ptr<SocketObject> obj);

protected: //TODO
  Connection();
  void Init();
  virtual std::shared_ptr<SocketContext> CreateSocketContext(bool from_accept);
  virtual bool SocketRead(std::shared_ptr<Client> obj, void* dest, int dest_size, int& out_read_size);
  virtual bool SocketWrite(std::shared_ptr<Client> obj, void* buffer, int size, int& out_write_size);

  int CreateServerSocket(int port);

  void ProcessSocket(std::shared_ptr<SocketObject> obj);
  void ProcessUnfinishedSocket(std::shared_ptr<SocketObject> obj);

  void Read(std::shared_ptr<Client> obj);
  bool Write(std::shared_ptr<Client> obj, std::shared_ptr<Message>);
  void Close(int socket_fd);

private: //TODO
  std::shared_ptr<Transporter> _transporter;
  std::shared_ptr<ConnectThread> _connector;
};
