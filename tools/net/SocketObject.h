/*
Copyright (c) 2018 - 2020 Adam Kaniewski

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

#include <memory>

class Connection;
class SocketContext;

class SocketObject : public std::enable_shared_from_this<SocketObject> {

public:
 SocketObject(int socket_fd,
              bool is_server_socket,
              std::shared_ptr<Connection> connection);
 virtual ~SocketObject();

 bool IsServerSocket();
 int SocketFd();
 std::shared_ptr<SocketContext> GetContext();
 std::shared_ptr<Connection> GetConnection();
 void SetContext(std::shared_ptr<SocketContext> context);
 bool IsActive();
 void SetActive(bool is_active);
 virtual void OnConnectionClosed();
protected:
 int _socket_fd;
 bool _is_server_socket;
 bool _is_active;
 bool _was_handle_closed;
 std::shared_ptr<SocketContext> _context;
 std::shared_ptr<Connection> _connection;
};
