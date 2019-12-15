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

#include "SocketObject.h"
#include "Connection.h"
#include "Logger.h"

SocketObject::SocketObject(int raw_handle,
                           bool is_server_socket,
                           std::shared_ptr<Connection> connection)
    : _raw_handle(raw_handle)
    , _is_server_socket(is_server_socket)
    , _is_active(true)
    , _connection(connection) {
}

SocketObject::~SocketObject() {
  _connection->Close(this);
}

bool SocketObject::IsServerSocket() {
  return _is_server_socket;
}

int SocketObject::Handle() {
  return _raw_handle;
}

std::shared_ptr<SessionInfo> SocketObject::GetSession() {
  return _session;
}

void SocketObject::SetSession(std::shared_ptr<SessionInfo> session) {
  _session = session;
}

bool SocketObject::IsActive() {
  return _is_active;
}

void SocketObject::SetActive(bool is_active) {
  _is_active = is_active;
}