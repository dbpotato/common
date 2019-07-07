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

#include "Server.h"
#include "Logger.h"

Server::Server(size_t raw_handle)
    : SocketObject(raw_handle, true)
    , _started(false) {
}

bool Server::Init(std::weak_ptr<ServerManager> mgr) {
  if(_started) {
    DLOG(error, "Server already started");
    return false;
  }
  _manager = mgr;
  _started = true;
  return true;
}

void Server::OnClientConnected(std::shared_ptr<Client> client) {
  if (client) {
    if(auto manager = _manager.lock())
      manager->OnClientConnected(client);
  }
}

bool Server::IsActive() {
  return _started;
}
