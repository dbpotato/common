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
#include "Client.h"
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

SessionInfoSSL::SessionInfoSSL(bool from_accept)
    : BaseSessionInfo(from_accept)
    , _ssl_handle(nullptr)
    , _read_pending(false) {
}

SessionInfoSSL::~SessionInfoSSL() {
  if(_ssl_handle) {
    SSL_free(_ssl_handle);
    _ssl_handle = nullptr;
  }
}

SSL* SessionInfoSSL::SSLHandle() {
    return _ssl_handle;
}

void SessionInfoSSL::SetSSLHandle(SSL* ssl) {
  _ssl_handle = ssl;
}

bool SessionInfoSSL::HasReadPending() {
  return _read_pending;
}

void SessionInfoSSL::SetReadPending(bool is_pending) {
  _read_pending = is_pending;
}

ConnectionSSL::ConnectionSSL(SSL_CTX* ctx)
    : _ctx(ctx) {
  SSL_CTX_set_mode(_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
}

ConnectionSSL::~ConnectionSSL() {
  if(_ctx)
    SSL_CTX_free(_ctx);
}

std::shared_ptr<BaseSessionInfo> ConnectionSSL::CreateSessionInfo(bool from_accept) {
  return std::make_shared<SessionInfoSSL>(from_accept);
}

NetError ConnectionSSL::AfterSocketCreated(std::shared_ptr<SocketObject> obj) {
  auto session = std::static_pointer_cast<SessionInfoSSL>(obj->GetSession());
  auto ssl_handle = session->SSLHandle();
  if(!ssl_handle) {
    ssl_handle = SSL_new(_ctx);
    SSL_set_fd(ssl_handle, obj->Handle());
    session->SetSSLHandle(ssl_handle);
  }

  int res = SSL_connect(ssl_handle);
  if(res == 1) {
    return NetError::OK;
  }


  int err = SSL_get_error(ssl_handle, res);
  if((err == SSL_ERROR_WANT_READ) ||
     (err == SSL_ERROR_WANT_WRITE)) {
    return NetError::RETRY;
  }

  DLOG(error, "ConnectionSSL::AfterSocketCreated SSL_connect fail : {}", SSLErrStr(ssl_handle, res));
  return NetError::FAILED;
}

NetError ConnectionSSL::AfterSocketAccepted(std::shared_ptr<SocketObject> obj) {
  auto session = std::static_pointer_cast<SessionInfoSSL>(obj->GetSession());
  auto ssl_handle = session->SSLHandle();
  if(!ssl_handle) {
    ssl_handle = SSL_new(_ctx);
    SSL_set_fd(ssl_handle, obj->Handle());
    session->SetSSLHandle(ssl_handle);
  }

  int res = SSL_accept(ssl_handle);
  if(res == 1)
    return NetError::OK;


  int err = SSL_get_error(ssl_handle, res);
  if((err == SSL_ERROR_WANT_READ) ||
     (err == SSL_ERROR_WANT_WRITE)) {
    return NetError::RETRY;
  }

  DLOG(error, "ConnectionSSL::AfterSocketAccepted SSL_accept fail : {}", SSLErrStr(ssl_handle, res));
  return NetError::FAILED;
}

bool ConnectionSSL::SocketRead(std::shared_ptr<Client> obj, void* dest, int dest_size, int& out_read_size) {
  auto session = std::static_pointer_cast<SessionInfoSSL>(obj->GetSession());
  auto ssl_handle = session->SSLHandle();
  bool result = true;
  int read_size = 0;
  int read_blocked = 0;

  read_size = SSL_read(ssl_handle, dest, dest_size);
  if(read_size <= 0) {
    out_read_size = 0;
    int err = SSL_get_error(ssl_handle, read_size);
    if((err == SSL_ERROR_WANT_READ) ||
       (err == SSL_ERROR_WANT_WRITE)) {
      read_blocked = 1;
    }
    else {
      DLOG(error, "ConnectionSSL SSL_read fail : {}", SSLErrStr(ssl_handle, read_size));
      result = false;
    }
  }

  session->SetReadPending(SSL_pending(ssl_handle) && !read_blocked);

  if(result)
    out_read_size = read_size;

  return result;
}

bool ConnectionSSL::SocketWrite(std::shared_ptr<Client> obj, void* buffer, int size, int& out_write_size) {
  auto session = std::static_pointer_cast<SessionInfoSSL>(obj->GetSession());
  auto ssl_handle = session->SSLHandle();

  size_t written = 0;

  int res = SSL_write(ssl_handle, buffer, size);
  if(res <= 0) {
    out_write_size = 0;
    int err = SSL_get_error(ssl_handle, res);
    if((err == SSL_ERROR_WANT_READ) ||
       (err == SSL_ERROR_WANT_WRITE)) {
      return true;
    }
    else {
      DLOG(error, "ConnectionSSL SSL_write fail : {}", SSLErrStr(ssl_handle, res));
      return false;
    }
  }
  else {
    out_write_size = res;
  }
  return true;
}
