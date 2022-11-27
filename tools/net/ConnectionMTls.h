/*
Copyright (c) 2022 Adam Kaniewski

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
#include "SocketContext.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl_cache.h"
#include "mbedtls/ssl.h"


class ConnectionMTls;

class MtlsPrivateKeyCert {
public:
  std::string _cert;
  std::string _key;
};

class MtlsCertSource {
public:
  std::string _cert_dir_path;
  std::string _cert_file;
  std::vector<std::string> _certs;
};

class SocketContextMtls : public SocketContext {
public:
  SocketContextMtls(SocketContext::State init_state, std::shared_ptr<Server> server);
  SocketContextMtls(SocketContext::State init_state, MtlsPrivateKeyCert& private_key, std::shared_ptr<ConnectionMTls> connection);
  ~SocketContextMtls();
  void SetSocketFd(int socket_fd);
  bool SetHostName(const std::string& host_name);
  bool HasReadPending() override;
  void SetReadPending(bool is_pending);
  NetError MakeHandshake();
  bool Write(void* buffer, int size, int& out_write_size);
  bool Read(void* dest, int dest_size, int& out_read_size);
  int GetLastSslError();
  bool IsSslVerified();
protected:
  SocketContextMtls(SocketContext::State init_state, std::shared_ptr<ConnectionMTls> connection);
  mbedtls_ssl_config& GetSslConfing();

private:
  bool SetSslConfig(mbedtls_ssl_config& ssl_conf);
  void SetPrivateKey(MtlsPrivateKeyCert& private_key, std::shared_ptr<ConnectionMTls> connection);

  bool _read_pending;
  bool _ssl_verify;
  int _last_ssl_error;
  int _last_read_value;
  mbedtls_ssl_context _ssl_context;
  mbedtls_ssl_config _ssl_conf;
  mbedtls_x509_crt _owned_cert;
  mbedtls_pk_context _pk_context;
  mbedtls_net_context _net_context;
};

class ConnectionMTls : public Connection {
public:
  static std::string GetErrorName(int error_code);
  static std::shared_ptr<ConnectionMTls> CreateMTls(MtlsCertSource& trusted_certs); 
 ~ConnectionMTls();

  void CreateClient(int port,
                    const std::string& host,
                    std::weak_ptr<ClientManager> mgr,
                    MtlsPrivateKeyCert& private_key);

  std::shared_ptr<Server> CreateServer(int port,
                                       std::shared_ptr<ClientManager> listener,
                                       MtlsPrivateKeyCert& private_key);

  std::shared_ptr<Server> CreateServer(int port,
                                       std::vector<std::weak_ptr<ClientManager> >& listeners,
                                       bool is_raw,
                                       MtlsPrivateKeyCert& private_key);

  NetError AfterSocketCreated(std::shared_ptr<SocketObject> obj) override;
  NetError AfterSocketAccepted(std::shared_ptr<SocketObject> obj) override;
  mbedtls_entropy_context& GetEntropy();
  mbedtls_ctr_drbg_context& GetCtrDrbg();
  mbedtls_x509_crt& GetCertChain();
protected:
  ConnectionMTls();
  bool Init(MtlsCertSource& trusted_certs);
  std::shared_ptr<SocketContext> CreateAcceptSocketContext(std::shared_ptr<Server> server) override;
  bool SocketRead(std::shared_ptr<Client> obj, void* dest, int dest_size, int& out_read_size) override;
  bool SocketWrite(std::shared_ptr<Client> obj, void* buffer, int size, int& out_write_size) override;
private:
  mbedtls_entropy_context _entropy;
  mbedtls_ctr_drbg_context _ctr_drbg;
  mbedtls_x509_crt _cert_chain;
};
