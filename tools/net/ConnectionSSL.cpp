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

#include "ConnectionSSL.h"
#include "Logger.h"
#include "Message.h"

#include "openssl/ssl.h"
#include "openssl/err.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

std::string SSLErrStr(const SSL* ssl, int res) {
  int err = SSL_get_error(ssl, res);
  switch(err) {
    case SSL_ERROR_NONE:
      return "SSL_ERROR_NONE";
    case SSL_ERROR_ZERO_RETURN:
        return "SSL_ERROR_ZERO_RETURN";
    case SSL_ERROR_WANT_READ:
        return "SSL_ERROR_WANT_READ";
    case SSL_ERROR_WANT_WRITE:
        return "SSL_ERROR_WANT_WRITE";
    case SSL_ERROR_WANT_CONNECT:
        return "SSL_ERROR_WANT_CONNECT";
    case SSL_ERROR_WANT_ACCEPT:
        return "SSL_ERROR_WANT_ACCEPT";
    case SSL_ERROR_WANT_X509_LOOKUP:
        return "SSL_ERROR_WANT_X509_LOOKUP";
    case SSL_ERROR_WANT_ASYNC:
        return "SSL_ERROR_WANT_ASYNC";
    case SSL_ERROR_WANT_ASYNC_JOB:
        return "SSL_ERROR_WANT_ASYNC_JOB";
    case SSL_ERROR_SYSCALL:
        return "SSL_ERROR_SYSCALL";
    case SSL_ERROR_SSL:
        return "SSL_ERROR_SSL";
    default:
        return "UNKNOWN VALUE";
  }
};

SessionInfoSSL::SessionInfoSSL(SSL* ssl_handle)
    : _ssl_handle(ssl_handle) {
}

SessionInfoSSL::~SessionInfoSSL() {
  SSL_free(_ssl_handle);
}

ConnectionSSL::ConnectionSSL(SSL_CTX* ctx)
    : _ctx(ctx) {
}

ConnectionSSL::~ConnectionSSL() {
  if(_ctx)
    SSL_CTX_free(_ctx);
}

bool ConnectionSSL::AfterSocketCreated(int socket, std::shared_ptr<SessionInfo>& session) {
  SSL* ssl = SSL_new(_ctx);
  SSL_set_fd(ssl, socket);
  int res = SSL_connect(ssl);
  if (res == 1) {
    session = std::make_shared<SessionInfoSSL>(ssl);
    return true;
  }

  DLOG(error, "ConnectionSSL::AfterSocketCreated SSL_connect fail : {}", SSLErrStr(ssl, res));
  return false;
}

bool ConnectionSSL::AfterSocketAccepted(int socket, std::shared_ptr<SessionInfo>& session) {
  SSL* ssl = SSL_new(_ctx);
  SSL_set_fd(ssl, socket);
  int res = SSL_accept(ssl);
  if(res == 1) {
    session = std::make_shared<SessionInfoSSL>(ssl);
    return true;
  }

  DLOG(error, "ConnectionSSL::AfterSocketCreated SSL_connect fail : {}", SSLErrStr(ssl, res));
  return false;
}

bool ConnectionSSL::SocketRead(std::shared_ptr<SocketObject> obj, void* dest, int dest_size, int& out_read_size) {
  auto ssl_session = std::static_pointer_cast<SessionInfoSSL>(obj->Session());
  SSL* ssl = ssl_session->Handle();
  int res = SSL_read(ssl, dest, dest_size);
  if(res < 0) {
    out_read_size = 0;
    int err = SSL_get_error(ssl, res);
    if((err == SSL_ERROR_WANT_READ) ||
       (err == SSL_ERROR_WANT_WRITE)) {
      return true;
    }
    else {
      DLOG(error, "ConnectionSSL SSL_read fail : {}", SSLErrStr(ssl, res));
      return false;
    }
  }
  else
    out_read_size = res;

  return true;
}

bool ConnectionSSL::SocketWrite(std::shared_ptr<SocketObject> obj, void* buffer, int size, int& out_write_size) {
  auto ssl_session = std::static_pointer_cast<SessionInfoSSL>(obj->Session());
  SSL* ssl = ssl_session->Handle();
  int res = SSL_write(ssl, buffer, size);
  if(res <= 0) {
    out_write_size = 0;
    int err = SSL_get_error(ssl, res);
    if((err == SSL_ERROR_WANT_READ) ||
       (err == SSL_ERROR_WANT_WRITE)) {
      return true;
    }
    else {
      DLOG(error, "ConnectionSSL SSL_write fail : {}", SSLErrStr(ssl, res));
      return false;
    }
  }
  else
    out_write_size = res;

  return true;
}
