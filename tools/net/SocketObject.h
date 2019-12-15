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

#include "Utils.h"

#include <memory>


class Message;
class Connection;
class Client;
class ClientManager;

class SessionInfo {
public:
  SessionInfo(){};
  virtual bool HasReadPending() {return false;}
};

class SocketObject : public std::enable_shared_from_this<SocketObject> {

public:
 SocketObject(int raw_handle,
              bool is_server_socket,
              std::shared_ptr<Connection> connection);
 virtual ~SocketObject();

 bool IsServerSocket();
 int Handle();
 std::shared_ptr<SessionInfo> GetSession();
 void SetSession(std::shared_ptr<SessionInfo> session);
 bool IsActive();
 void SetActive(bool is_active);
protected:
 int _raw_handle;
 bool _is_server_socket;
 bool _is_active;
 std::shared_ptr<SessionInfo> _session;
 std::shared_ptr<Connection> _connection;
};
