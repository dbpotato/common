/*
Copyright (c) 2022 - 2024 Adam Kaniewski

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
#include "MtlsCppWrapper.h"
#include "Server.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace MtlsCppWrapper;


SocketContextMtls::SocketContextMtls(SocketContext::State init_state, int socket_fd, std::shared_ptr<Server> server)
    : SocketContext(init_state)
    , _ssl_verify(false) {
  auto server_context = std::static_pointer_cast<SocketContextMtls>(server->GetContext());
  _ssl_conf = server_context->GetSslConfing();
  _ssl = MtlsCppSsl::CreateSsl(socket_fd, _ssl_conf);
  if(!_ssl) {
    DLOG(error, "FAILED to create SSL");
  }
}

SocketContextMtls::SocketContextMtls(SocketContext::State init_state,
                                   std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> config)
    : SocketContext(init_state)
    , _ssl_verify(false)
    , _ssl_conf(config) {
}

SocketContextMtls::~SocketContextMtls() {
}

bool SocketContextMtls::SetSocket(int socket_fd, const std::string& host_name) {
  if(_ssl) {
    return true;
  }
  _ssl = MtlsCppSsl::CreateSsl(socket_fd, _ssl_conf, host_name);
  if(!_ssl) {
    DLOG(error, "FAILED to create SSL");
    return false;
  }
  return true;
}

std::shared_ptr<MtlsCppConfig> SocketContextMtls::GetSslConfing() {
  return _ssl_conf;
}

bool SocketContextMtls::IsSslVerified() {
  return _ssl_verify;
}

NetError SocketContextMtls::MakeHandshake() {
  if(!_ssl) {
    return NetError::FAILED;
  }
  return _ssl->MakeHandshake(_ssl_verify);
}

bool SocketContextMtls::Write(const void* buffer, int size, size_t& out_write_size) {
  if(!_ssl) {
    return false;
  }
  return _ssl->Write(buffer, size, out_write_size);
}

bool SocketContextMtls::Read(void* dest, int dest_size, size_t& out_read_size) {
  if(!_ssl) {
    return false;
  }
  return _ssl->Read(dest, dest_size, out_read_size);
}

std::shared_ptr<ConnectionMTls> ConnectionMTls::CreateMTls(std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> default_client_config) {
  std::shared_ptr<ConnectionMTls> connection;
  connection.reset(new ConnectionMTls(default_client_config));
  if(connection->Init()) {
    return connection;
  }
  return nullptr;
}

std::shared_ptr<ConnectionMTls> ConnectionMTls::CreateMTls() {
  return CreateMTls(MtlsCppConfig::CreateClientConfig());
}

ConnectionMTls::ConnectionMTls(std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> default_client_config)
  : _default_client_config(default_client_config) {
}

ConnectionMTls::~ConnectionMTls() {
}

bool ConnectionMTls::Init() {
  if(!Connection::Init()){
    return false;
  }

  if(_default_client_config && _default_client_config->IsServerConfig()) {
    DLOG(error, "Creating instance with config for server");
    return false;
  }

  if(!_default_client_config) {
    _default_client_config = MtlsCppConfig::CreateClientConfig();
  }

  return (_default_client_config != nullptr);
}

void ConnectionMTls::CreateClient(int port,
                                  const std::string& host,
                                  std::weak_ptr<ClientManager> mgr) {
  CreateClient(port, host, mgr, _default_client_config);
}

void ConnectionMTls::CreateClient(int port,
                                  const std::string& host,
                                  std::weak_ptr<ClientManager> mgr,
                                  std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> config) {
  auto context = std::make_shared<SocketContextMtls>(SocketContext::State::GETTING_INFO, config);
  Connection::CreateClient(port,
                           host,
                           mgr,
                           context);
}

std::shared_ptr<Server> ConnectionMTls::CreateServer(int port,
                                     std::shared_ptr<ClientManager> listener,
                                     std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> config) {
  std::vector<std::weak_ptr<ClientManager> > listeners;
  listeners.push_back(listener);
  return CreateServer(port, listeners, config);
}

std::shared_ptr<Server> ConnectionMTls::CreateServer(int port,
                                     std::vector<std::weak_ptr<ClientManager> >& listeners,
                                     std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> config) {
  auto context = std::make_shared<SocketContextMtls>(SocketContext::State::FINISHED, config);
  std::shared_ptr<Server> server = Connection::CreateServer(port, listeners, context);
  return server;
}

std::shared_ptr<SocketContext> ConnectionMTls::CreateAcceptSocketContext(int socket_fd, std::shared_ptr<Server> server) {
  return std::make_shared<SocketContextMtls>(SocketContext::State::AFTER_ACCEPTING, socket_fd, server);
}

NetError ConnectionMTls::AfterSocketCreated(std::shared_ptr<SocketObject> obj) {
  auto context = std::static_pointer_cast<SocketContextMtls>(obj->GetContext());
  auto client = std::static_pointer_cast<Client>(obj);

  if(!context->SetSocket(obj->GetFd(), client->GetUrl())){
    return NetError::FAILED;
  }
  return context->MakeHandshake();
}

NetError ConnectionMTls::AfterSocketAccepted(std::shared_ptr<SocketObject> obj) {
  auto context = std::static_pointer_cast<SocketContextMtls>(obj->GetContext());
  return context->MakeHandshake();
}

bool ConnectionMTls::SocketRead(std::shared_ptr<Client> obj, void* dest, int dest_size, size_t& out_read_size) {
  auto context = std::static_pointer_cast<SocketContextMtls>(obj->GetContext());
  return context->Read(dest, dest_size, out_read_size);
}

bool ConnectionMTls::SocketWrite(std::shared_ptr<Client> obj, const void* buffer, int size, size_t& out_write_size) {
  auto context = std::static_pointer_cast<SocketContextMtls>(obj->GetContext());
  return context->Write(buffer, size, out_write_size);
}
