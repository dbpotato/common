/*
Copyright (c) 2018 - 2024 Adam Kaniewski

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

#include "Epool.h"

#include <memory>

class Connection;
class SocketContext;

class SocketObject : public std::enable_shared_from_this<SocketObject>
                   , public FdListener {

public:
 SocketObject(int socket_fd,
              bool is_server_socket,
              std::shared_ptr<Connection> connection,
              std::shared_ptr<SocketContext> context);
 virtual ~SocketObject();

 bool IsServerSocket();
 std::shared_ptr<SocketContext> GetContext();
 std::shared_ptr<Connection> GetConnection();
 bool IsActive();
 bool IsValid();
 void SetActive(bool is_active);
 virtual void OnConnectionClosed();

 //FdListener
 int GetFd() override;
 void OnFdReadReady() override;
 void OnFdWriteReady() override;
 void OnFdOperationError(bool is_epool_err) override;

protected:
 int _socket_fd;
 bool _is_server_socket;
 bool _is_active;
 bool _was_fd_closed;
 std::shared_ptr<Connection> _connection;
 std::shared_ptr<SocketContext> _context;
};
