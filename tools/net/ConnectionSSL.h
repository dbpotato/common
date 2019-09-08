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

#include "openssl/ssl.h"

class SessionInfoSSL : public SessionInfo {
public:
  SessionInfoSSL(SSL* ssl_handle);
  ~SessionInfoSSL();
  SSL* Handle(){return _ssl_handle;}
private :
  SSL* _ssl_handle;
};

class ConnectionSSL : public Connection {
public:
  ConnectionSSL(SSL_CTX* ctx);
 ~ConnectionSSL();
protected:
  bool AfterSocketCreated(int socket, std::shared_ptr<SessionInfo>& session) override;
  bool AfterSocketAccepted(int socket, std::shared_ptr<SessionInfo>& session) override;
  bool SocketRead(std::shared_ptr<SocketObject> obj, void* dest, int dest_size, int& out_read_size) override;
  bool SocketWrite(std::shared_ptr<SocketObject> obj, void* buffer, int size, int& out_write_size) override;
private:
  SSL_CTX* _ctx;
};
