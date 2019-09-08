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

#include "Connection.h"
#include "SocketObject.h"

class Client;

class ServerManager {
public:
  virtual void OnClientConnected(std::shared_ptr<Client> client) = 0;
};

class Server : public SocketObject {

friend class std::shared_ptr<Server> Connection::CreateServer(int);

public:
  bool Init(std::weak_ptr<ServerManager> mgr);
  void OnClientConnected(std::shared_ptr<Client> client);
  bool IsActive() override;
protected:
  std::weak_ptr<ServerManager> _manager;
private:
  Server(size_t raw_handle, std::shared_ptr<Connection> connection);
  bool _started;
};
