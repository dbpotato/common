/*
Copyright (c) 2022 - 2023 Adam Kaniewski

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


class ConnectionMTls;
namespace MtlsCppWrapper {
  class MtlsCppConfig;
  class MtlsCppSsl;
}


class SocketContextMtls : public SocketContext {
public:
  SocketContextMtls(SocketContext::State init_state,
                  int socket_fd,
                  std::shared_ptr<Server> server);

  SocketContextMtls(SocketContext::State init_state,
                  std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> config);

  ~SocketContextMtls();
  bool SetSocket(int socket_fd, const std::string& host_name);
  NetError MakeHandshake();
  bool Write(const void* buffer, int size, size_t& out_write_size);
  bool Read(void* dest, int dest_size, size_t& out_read_size);
  bool IsSslVerified();
protected:
  std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> GetSslConfing();
private:
  bool _ssl_verify;
  std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> _ssl_conf;
  std::shared_ptr<MtlsCppWrapper::MtlsCppSsl> _ssl;
};

class ConnectionMTls : public Connection {
public:
  static std::shared_ptr<ConnectionMTls> CreateMTls();
  static std::shared_ptr<ConnectionMTls> CreateMTls(std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> default_client_config); 
 ~ConnectionMTls();

  void CreateClient(int port,
                    const std::string& host,
                    std::weak_ptr<ClientManager> mgr) override;

  void CreateClient(int port,
                    const std::string& host,
                    std::weak_ptr<ClientManager> mgr,
                    std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> config);

  std::shared_ptr<Server> CreateServer(int port,
                                       std::shared_ptr<ClientManager> listener,
                                       std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> config);

  std::shared_ptr<Server> CreateServer(int port,
                                       std::vector<std::weak_ptr<ClientManager> >& listeners,
                                       std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> config);

  NetError AfterSocketCreated(std::shared_ptr<SocketObject> obj) override;
  NetError AfterSocketAccepted(std::shared_ptr<SocketObject> obj) override;

protected:
  using Connection::CreateServer;

  ConnectionMTls(std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> default_client_config);
  bool Init() override;
  std::shared_ptr<SocketContext> CreateAcceptSocketContext(int socket_fd, std::shared_ptr<Server> server) override;
  bool SocketRead(std::shared_ptr<Client> obj, void* dest, int dest_size, size_t& out_read_size) override;
  bool SocketWrite(std::shared_ptr<Client> obj, const void* buffer, int size, size_t& out_write_size) override;
  std::shared_ptr<MtlsCppWrapper::MtlsCppConfig> _default_client_config;
};
