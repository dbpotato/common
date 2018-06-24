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
#include "Connection.h"

class Client;

class ServerManager {
public:
  virtual void OnClientConnected(std::shared_ptr<Client> client) = 0;
};

class Server : public std::enable_shared_from_this<Server>, public ThreadObject {

friend class std::shared_ptr<Server> Connection::CreateServer(int);

public:
  ~Server();
  bool Init(std::weak_ptr<ServerManager> mgr);
protected:
  //ThreadObject
  void OnThreadStarted(int thread_id) override;

  void Listen();

  std::weak_ptr<ServerManager> _manager;
  std::shared_ptr<Connection> _connection;
private:
  Server(int socket, std::shared_ptr<Connection> connection);
  PosixThread _run_thread;
  int _socket;
  bool _started;
};
