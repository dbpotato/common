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

#include <memory>

class Message;
class Connection;

class Data {
public:
  Data();
  uint32_t _size;
  std::shared_ptr<unsigned char> _data;
};

class SessionInfo {
public:
  SessionInfo(){};
};

class SocketObject : public std::enable_shared_from_this<SocketObject> {

public:
 SocketObject(size_t raw_handle,
              bool is_server_socket,
              std::shared_ptr<Connection> connection);
 virtual ~SocketObject();
 
 virtual void OnMsgWrite(std::shared_ptr<Message> msg, bool status);
 virtual void OnDataRead(Data& data);
 virtual void OnConnectionClosed();
 virtual bool IsActive();

 bool IsServerSocket();
 size_t Handle();
 std::shared_ptr<SessionInfo> Session();
 void SetSession(std::shared_ptr<SessionInfo> session);
protected:
 size_t _raw_handle;
 bool _is_server_socket;
 std::shared_ptr<SessionInfo> _session;
 std::shared_ptr<Connection> _connection;
};
