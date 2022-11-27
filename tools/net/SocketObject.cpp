/*
Copyright (c) 2018 - 2022 Adam Kaniewski

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

#include "SocketObject.h"
#include "Connection.h"
#include "Utils.h"

SocketObject::SocketObject(int socket_fd,
                           bool is_server_socket,
                           std::shared_ptr<Connection> connection,
                           std::shared_ptr<SocketContext> context)
    : _socket_fd(socket_fd)
    , _is_server_socket(is_server_socket)
    , _is_active(true)
    , _was_fd_closed(false)
    , _connection(connection)
    , _context(context) {
}

SocketObject::~SocketObject() {
  if(!_was_fd_closed && IsValid()) {
    _connection->Close(_socket_fd);
  }
}

bool SocketObject::IsServerSocket() {
  return _is_server_socket;
}

int SocketObject::SocketFd() {
  return _socket_fd;
}

std::shared_ptr<SocketContext> SocketObject::GetContext() {
  return _context;
}

std::shared_ptr<Connection> SocketObject::GetConnection() {
  return _connection;
}

bool SocketObject::IsActive() {
  return _is_active;
}

bool SocketObject::IsValid() {
  return (_socket_fd != DEFAULT_SOCKET) && !_was_fd_closed;
}

void SocketObject::SetActive(bool is_active) {
  if((_is_active != is_active) && IsValid()) {
    _is_active = is_active;
    _connection->NotifySocketActiveChanged(shared_from_this());
  }
}

void SocketObject::OnConnectionClosed() {
  _was_fd_closed = true;
}
