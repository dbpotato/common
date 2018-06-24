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

#include "Server.h"
#include "Logger.h"

timeval default_listen_timeout() {
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  return timeout;
}

Server::Server(int socket, std::shared_ptr<Connection> connection)
    : _connection(connection)
    , _socket(socket)
    , _started(false){
}

Server::~Server() {
  _run_thread.Stop();
  _run_thread.Join();
  _connection->Close(_socket);
}

bool Server::Init(std::weak_ptr<ServerManager> mgr) {
  if(_started) {
    log()->error("Server already started");
    return false;
  }

  _manager = mgr;
  _started = true;

  _run_thread.Run(shared_from_this());
  return true;
}

void Server::OnThreadStarted(int thread_id) {
  Listen();
}

void Server::Listen() {
  while(_run_thread.ShouldRun()) {
     std::shared_ptr<Client> client = _connection->Accept(_socket);
     if (client) {
       if(auto manager = _manager.lock()) {
         manager->OnClientConnected(client);
       }
     }
     else {
       log()->error("Server: accept failed");
     }
  }
}
