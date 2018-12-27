#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "ConnectionSSL.h"
#include "Logger.h"
#include "Message.h"

#include "openssl/ssl.h"
#include "openssl/err.h"

#ifndef SO_REUSEPORT
#define SO_REUSEPORT 15
#endif

ConnectionSSL::ConnectionSSL(SSL_CTX* ctx)
    : _ctx(ctx) {
}

int ConnectionSSL::AfterSocketCreated(int soc, bool listen_soc) {
  if (listen_soc)
    return soc;

  SSL* ssl = SSL_new(_ctx);
  SSL_set_fd(ssl, soc);
  if (SSL_connect(ssl))
    _sessions.insert(std::make_pair(soc, ssl));
  else {
    DLOG(error, "ConnectionSSL::AfterSocketCreated SSL_connect fail");
    close(soc);
    return -1;
  }

  return soc;
}

int ConnectionSSL::AfterSocketAccepted(int soc) {
  if(soc > -1) {
    SSL* ssl = SSL_new(_ctx);
    SSL_set_fd(ssl, soc);
    int accept_res = SSL_accept(ssl);
    if(accept_res != 1) {
      DLOG(error, "ConnectionSSL::Accept SSL_accept fail : {}", accept_res);
      close(soc);
      soc = -1;
    }
    else {
      _sessions.insert(std::make_pair(soc, ssl));
    }
  }
  return soc;
}

void ConnectionSSL::Close(int soc) {
  auto it = _sessions.find(soc);
  if(it != _sessions.end()) {
    SSL_free(it->second);
    _sessions.erase(it);
  }
  else
    DLOG(error, "ConnectionSSL::Close ssl session not found");

  close(soc);
}

int ConnectionSSL::SocketRead(int soc, void* dest, int dest_lenght) {
  SSL* ssl = _sessions[soc];
  return SSL_read(ssl, dest, dest_lenght);
}

int ConnectionSSL::SocketWrite(int soc, void* buffer, int size) {
  SSL* ssl = _sessions[soc];
  return SSL_write(ssl, buffer, size);
}


