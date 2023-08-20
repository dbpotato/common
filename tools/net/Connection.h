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

#include "SocketObject.h"
#include "Utils.h"

#include <map>
#include <string>
#include <vector>


class Message;
class MessageWriteRequest;
class Client;
class ClientManager;
class ClientOwner;
class Data;
class Server;
class SocketContext;
class ThreadLoop;
class Epool;
class ConnectThread;


class Connection : public std::enable_shared_from_this<Connection> {

friend class Client;
friend class Epool;
friend class SocketObject;

public:
  static std::shared_ptr<Connection> CreateBasic();
  virtual ~Connection();

  virtual void CreateClient(int port,
                    const std::string& host,
                    std::weak_ptr<ClientManager> mgr);

  std::shared_ptr<Server> CreateServer(int port,
                                       std::shared_ptr<ClientManager> listener);

  std::shared_ptr<Server> CreateServer(int port,
                                       std::vector<std::weak_ptr<ClientManager> >& listeners);

  std::shared_ptr<Server> CreateServer(int port,
                                       std::vector<std::weak_ptr<ClientManager> >& listeners,
                                       std::shared_ptr<SocketContext> context);

  virtual NetError AfterSocketCreated(std::shared_ptr<SocketObject> obj);
  virtual NetError AfterSocketAccepted(std::shared_ptr<SocketObject> obj);

  void OnSocketReadReady(std::shared_ptr<SocketObject> obj);
  void OnSocketWriteReady(std::shared_ptr<SocketObject> obj);

protected:
  Connection();
  virtual bool Init();

  void CreateClient(int port,
                    const std::string& host,
                    std::weak_ptr<ClientManager> mgr,
                    std::shared_ptr<SocketContext> context);

  virtual std::shared_ptr<SocketContext> CreateAcceptSocketContext(int socket_fd, std::shared_ptr<Server> server);
  virtual bool SocketRead(std::shared_ptr<Client> obj, void* dest, int dest_size, size_t& out_read_size);
  virtual bool SocketWrite(std::shared_ptr<Client> obj, const void* buffer, int size, size_t& out_write_size);

  int CreateServerSocket(int port);

  void OnSocketClosed(std::shared_ptr<SocketObject> obj);
  void OnSocketDestroyed(int socket_fd);

  bool Read(std::shared_ptr<Client> obj);
  bool Write(std::shared_ptr<Client> obj, MessageWriteRequest& req, bool& completed);

private:
  void SendMsg(std::shared_ptr<Client>, std::shared_ptr<Message> msg);
  void Accept(std::shared_ptr<SocketObject> obj);
  void NotifySocketActiveChanged(std::shared_ptr<SocketObject> obj);
  bool HasObjectPendingWrite(std::shared_ptr<SocketObject> obj);
  std::shared_ptr<Epool> _epool;
  std::shared_ptr<ConnectThread> _connector;
  std::shared_ptr<ThreadLoop> _thread_loop;
  std::map<int, std::vector<MessageWriteRequest>> _write_reqs;
};
