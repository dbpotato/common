#pragma once

#include <map>
#include "openssl/ssl.h"

#include "Connection.h"

class ConnectionSSL : public Connection {
public:
  ConnectionSSL(SSL_CTX* ctx);
protected:
  int AfterSocketCreated(int soc, bool listen_soc) override;
  int AfterSocketAccepted(int soc) override;
  void Close(int soc) override;
  int SocketRead(int soc, void* dest, int dest_lenght) override;
  int SocketWrite(int soc, void* buffer, int size) override;

private:
  std::map<int, SSL*> _sessions;
  SSL_CTX* _ctx;
};
