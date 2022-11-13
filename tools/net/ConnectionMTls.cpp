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

#include "ConnectionMTls.h"
#include "Client.h"
#include "Logger.h"
#include "Message.h"
#include "Server.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


SocketContextMtls::SocketContextMtls(SocketContext::State init_state, std::shared_ptr<ConnectionMTls> connection) 
    : SocketContext(init_state)
    , _read_pending(false)
    , _ssl_verify(false)
    , _last_ssl_error(0) {

  mbedtls_ssl_init(&_ssl_context);
  mbedtls_ssl_config_init(&_ssl_conf);
  mbedtls_x509_crt_init(&_owned_cert);
  mbedtls_pk_init(&_pk_context);
  _net_context.fd = -1;

  mbedtls_ssl_config_defaults(&_ssl_conf,
                              (GetState() == SocketContext::State::GETTING_INFO) ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER,
                              MBEDTLS_SSL_TRANSPORT_STREAM,
                              MBEDTLS_SSL_PRESET_DEFAULT);

  mbedtls_ssl_conf_rng(&_ssl_conf, mbedtls_ctr_drbg_random, &connection->GetCtrDrbg());
  mbedtls_ssl_conf_ca_chain(&_ssl_conf, &connection->GetCertChain(), nullptr);
  mbedtls_ssl_conf_authmode(&_ssl_conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
}

SocketContextMtls::SocketContextMtls(SocketContext::State init_state, std::shared_ptr<Server> server) 
    : SocketContextMtls(init_state, std::static_pointer_cast<ConnectionMTls>(server->GetConnection())) {
  auto context = std::static_pointer_cast<SocketContextMtls>(server->GetContext());
  SetSslConfig(context->GetSslConfing());
}

SocketContextMtls::SocketContextMtls(SocketContext::State init_state, MtlsPrivateKeyCert& private_key, std::shared_ptr<ConnectionMTls> connection)
    : SocketContextMtls(init_state, connection) {
  SetPrivateKey(private_key, connection);
  SetSslConfig(_ssl_conf);
}

SocketContextMtls::~SocketContextMtls() {
    mbedtls_ssl_free(&_ssl_context);
    mbedtls_ssl_config_free(&_ssl_conf);
    mbedtls_x509_crt_free(&_owned_cert);
    mbedtls_pk_free(&_pk_context);
}

mbedtls_ssl_config& SocketContextMtls::GetSslConfing() {
  return _ssl_conf;
}

int SocketContextMtls::GetLastSslError() {
  return _last_ssl_error;
}

bool SocketContextMtls::IsSslVerified() {
  return _ssl_verify;
}

void SocketContextMtls::SetPrivateKey(MtlsPrivateKeyCert& private_key, std::shared_ptr<ConnectionMTls> connection) {
  if(!private_key._key.empty() && !private_key._cert.empty()) {
    if((_last_ssl_error = mbedtls_pk_parse_key(&_pk_context,
                        (const unsigned char *) private_key._key.c_str(),
                        private_key._key.length() + 1,
                        nullptr,
                        0,
                        mbedtls_ctr_drbg_random,
                        &connection->GetCtrDrbg())) != 0) {
      DLOG(error, "SocketContextMtls : owned key parse FAILED");
      return;
    }

    if((_last_ssl_error = mbedtls_x509_crt_parse(&_owned_cert,
                              (const unsigned char*)private_key._cert.c_str(),
                              private_key._cert.length() + 1)) != 0) {
      DLOG(error, "SocketContextMtls : owned cert parse FAILED");
      return;
    }

    if((_last_ssl_error = mbedtls_pk_check_pair(&(_owned_cert.pk),
                             &_pk_context,
                             mbedtls_ctr_drbg_random,
                             &connection->GetCtrDrbg())) != 0) {
      DLOG(error, "SocketContextMtls : key pair check FAILED");
      return;
    }

    if((_last_ssl_error = mbedtls_ssl_conf_own_cert( &_ssl_conf, &_owned_cert, &_pk_context)) != 0) {
      DLOG(error, "SocketContextMtls : Set own cert FAILED");
      return;
    }
  }
}

bool SocketContextMtls::SetSslConfig(mbedtls_ssl_config& ssl_conf) {
  if((_last_ssl_error = mbedtls_ssl_setup(&_ssl_context, &ssl_conf)) != 0) {
    DLOG(error, "SocketContextMtls : ssl setup FAILED");
    return false;
  }
  return true;
}

bool SocketContextMtls::HasReadPending() {
  return _read_pending;
}

void SocketContextMtls::SetReadPending(bool is_pending) {
  _read_pending = is_pending;
}

void SocketContextMtls::SetSocketFd(int socket_fd) {
  if(_net_context.fd == -1 ) {
    _net_context.fd = socket_fd;
    mbedtls_ssl_set_bio(&_ssl_context, &_net_context, mbedtls_net_send, mbedtls_net_recv, nullptr);
  }
}

bool SocketContextMtls::SetHostName(const std::string& host_name) {
  if((_last_ssl_error = mbedtls_ssl_set_hostname( &_ssl_context, host_name.c_str())) != 0) {
    DLOG(error, "SocketContextMtls::SetHostName Failed for : {}", host_name);
    return false;
  }
  return true;
}

NetError SocketContextMtls::MakeHandshake() {
  int result = -1;
  while(result != 0) {
    result = mbedtls_ssl_handshake(&_ssl_context);
    if(!result) {
      break;
    }
    if(result != MBEDTLS_ERR_SSL_WANT_READ && result != MBEDTLS_ERR_SSL_WANT_WRITE) {
      DLOG(error, "SocketContextMtls : Handshake FAILED");
      _last_ssl_error = result;
      return NetError::FAILED;
    }
  }

  if(!(_last_ssl_error = mbedtls_ssl_get_verify_result(&_ssl_context))) {
    _ssl_verify = true;
  }

  return NetError::OK;
}

bool SocketContextMtls::Write(void* buffer, int size, int& out_write_size) {
  int result = mbedtls_ssl_write(&_ssl_context, (const unsigned char*)buffer, size);
  if( result<= 0) {
    out_write_size = 0;
    if( result == MBEDTLS_ERR_SSL_WANT_READ ||
        result == MBEDTLS_ERR_SSL_WANT_WRITE ) {
      return true;
    } else {
      _last_ssl_error = result;
      return false;
    }
  }
  out_write_size = result;
  return true;
}

bool SocketContextMtls::Read(void* dest, int dest_size, int& out_read_size) {
  int read_value = mbedtls_ssl_read(&_ssl_context, (unsigned char*)dest, dest_size);
  if((read_value == MBEDTLS_ERR_SSL_WANT_READ) || (read_value == MBEDTLS_ERR_SSL_WANT_WRITE)) {
    _read_pending = true;
    out_read_size = 0;
    return true;
  } else if(read_value <= 0) {
    if(read_value < 0) {
      _last_ssl_error = read_value;
    }
    return false;
  }
  _read_pending = false;
  out_read_size = read_value;
  return true;
}

std::shared_ptr<ConnectionMTls> ConnectionMTls::CreateMTls(MtlsCertSource& trusted_certs) {
  std::shared_ptr<ConnectionMTls> connection;
  connection.reset(new ConnectionMTls());
  if(connection->Init(trusted_certs)) {
    return connection;
  }
  return nullptr;
}

ConnectionMTls::ConnectionMTls() {
  mbedtls_entropy_init(&_entropy);
  mbedtls_ctr_drbg_init(&_ctr_drbg);
  mbedtls_x509_crt_init(&_cert_chain);
}

ConnectionMTls::~ConnectionMTls() {
  mbedtls_x509_crt_free(&_cert_chain);
  mbedtls_ctr_drbg_free(&_ctr_drbg);
  mbedtls_entropy_free(&_entropy);
}

mbedtls_entropy_context& ConnectionMTls::GetEntropy() {
  return _entropy;
}

mbedtls_ctr_drbg_context& ConnectionMTls::GetCtrDrbg() {
  return _ctr_drbg;
}

mbedtls_x509_crt& ConnectionMTls::GetCertChain() {
  return _cert_chain;
}

std::string ConnectionMTls::GetErrorName(int error_code) {
  char error_buf[100];
  mbedtls_strerror(error_code, error_buf, 100);
  std::string result(error_buf);
  return result.empty() ? ("Unkonwn error code : " + std::to_string(error_code)) : result;
}

bool ConnectionMTls::Init(MtlsCertSource& trusted_certs) {
  Connection::Init();

  if(mbedtls_ctr_drbg_seed(&_ctr_drbg,
                           mbedtls_entropy_func,
                           &_entropy,
                           nullptr,
                           0) != 0) {
    DLOG(error, "ConnectionMTls : Fail mbedtls_ctr_drbg_seed");
    return false;
  }

  bool cert_parse_result = true;

  for(auto cert : trusted_certs._certs) {
    if(mbedtls_x509_crt_parse(&_cert_chain,
                              (const unsigned char*)cert.c_str(),
                              cert.length() + 1) != 0) {
      cert_parse_result = false;
    }
  }

  if(!trusted_certs._cert_file.empty()) {
    if(mbedtls_x509_crt_parse_file(&_cert_chain, trusted_certs._cert_file.c_str()) !=0) {
      cert_parse_result = false;
    }
  }

  if(!trusted_certs._cert_dir_path.empty()) {
    if(mbedtls_x509_crt_parse_path(&_cert_chain, trusted_certs._cert_dir_path.c_str()) !=0) {
      cert_parse_result = false;
    }
  }

  if(!cert_parse_result) {
    DLOG(error, "ConnectionMTls : cert parse FAILED");
  }

  return cert_parse_result;
}

void ConnectionMTls::CreateClient(int port,
                                  const std::string& host,
                                  std::weak_ptr<ClientManager> mgr,
                                  MtlsPrivateKeyCert& private_key) {
  auto sptr = std::static_pointer_cast<ConnectionMTls>(shared_from_this());
  auto context = std::make_shared<SocketContextMtls>(SocketContext::State::GETTING_INFO, private_key, sptr);
  Connection::CreateClient(port,
                           host,
                           mgr,
                           context);
}

std::shared_ptr<Server> ConnectionMTls::CreateServer(int port,
                                     std::shared_ptr<ClientManager> listener,
                                     MtlsPrivateKeyCert& private_key) {
  std::vector<std::weak_ptr<ClientManager> > listeners;
  listeners.push_back(listener);
  return CreateServer(port, listeners, listener->IsRaw(), private_key);
}

std::shared_ptr<Server> ConnectionMTls::CreateServer(int port,
                                     std::vector<std::weak_ptr<ClientManager> >& listeners,
                                     bool is_raw,
                                     MtlsPrivateKeyCert& private_key) {
  auto sptr = std::static_pointer_cast<ConnectionMTls>(shared_from_this());
  auto context = std::make_shared<SocketContextMtls>(SocketContext::State::FINISHED, private_key, sptr);
  std::shared_ptr<Server> server = Connection::CreateServer(port, listeners, is_raw, context);
  return server;
}

std::shared_ptr<SocketContext> ConnectionMTls::CreateAcceptSocketContext(std::shared_ptr<Server> server) {
  return std::make_shared<SocketContextMtls>(SocketContext::State::AFTER_ACCEPTING, server);
}

NetError ConnectionMTls::AfterSocketCreated(std::shared_ptr<SocketObject> obj) {
  auto context = std::static_pointer_cast<SocketContextMtls>(obj->GetContext());
  context->SetSocketFd(obj->SocketFd());

  auto client = std::static_pointer_cast<Client>(obj);
  if(context->SetHostName(client->GetUrl())) {
    return context->MakeHandshake();
  }

  return NetError::FAILED;
}

NetError ConnectionMTls::AfterSocketAccepted(std::shared_ptr<SocketObject> obj) {
  auto context = std::static_pointer_cast<SocketContextMtls>(obj->GetContext());
  context->SetSocketFd(obj->SocketFd());
  return context->MakeHandshake();
}

bool ConnectionMTls::SocketRead(std::shared_ptr<Client> obj, void* dest, int dest_size, int& out_read_size) {
  auto context = std::static_pointer_cast<SocketContextMtls>(obj->GetContext());
  return context->Read(dest, dest_size, out_read_size);
}

bool ConnectionMTls::SocketWrite(std::shared_ptr<Client> obj, void* buffer, int size, int& out_write_size) {
  auto context = std::static_pointer_cast<SocketContextMtls>(obj->GetContext());
  return context->Write(buffer, size, out_write_size);
}
