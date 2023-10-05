/*
Copyright (c) 2023 Adam Kaniewski

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

#include "Connection.h"
#include "Logger.h"
#include "Client.h"
#include "Data.h"
#include "Message.h"
#include "Config.h"
#include "Server.h"

#ifdef ENABLE_SSL
  #include "ConnectionMTls.h"
  #include "MtlsCppWrapper.h"

  using namespace MtlsCppWrapper;
#endif //ENABLE_SSL


class ServerListener : public ClientManager
                     , public std::enable_shared_from_this<ServerListener> {
public :
  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  void OnClientConnected(std::shared_ptr<Client> client) override;
  void OnClientClosed(std::shared_ptr<Client> client) override;
private :
  bool IsClientVerified(std::shared_ptr<Client> client);
};

void ServerListener::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  log()->info("Client {} : Read : {}", client->GetId(), msg->GetData()->ToString());
  log()->info("Sending Response : {}", Config::MSG_FOR_CLIENT);
  client->Send(std::make_shared<Message>(Config::MSG_FOR_CLIENT));
}

bool ServerListener::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  bool verified = IsClientVerified(client);
  log()->info("Client {} : Connecting : err : {} , ssl verified : {}",
              client->GetId(),
              (int)err,
              verified);
  return true;
}

void ServerListener::OnClientConnected(std::shared_ptr<Client> client) {
  log()->info("Client {} : Connected", client->GetId());
}

void ServerListener::OnClientClosed(std::shared_ptr<Client> client) {
  log()->info("Client {} : Closed", client->GetId());
}

bool ServerListener::IsClientVerified(std::shared_ptr<Client> client) {
  bool result = false;
#ifdef ENABLE_SSL
  auto context = std::static_pointer_cast<SocketContextMtls>(client->GetContext());
  result = context->IsSslVerified();
#endif //ENABLE_SSL
  return result;
}


std::shared_ptr<Server> CreateServer(std::shared_ptr<ServerListener> server_listener) {
  std::shared_ptr<Server> result;
#ifdef ENABLE_SSL
  auto ssl_config = MtlsCppConfig::CreateServerConfig(Config::SERVER_KEY, Config::SERVER_CERT, Config::AUTH_CERT);
  auto connection = ConnectionMTls::CreateMTls();
  result = connection->CreateServer(Config::SERVER_PORT, server_listener, ssl_config);
#else
  auto connection = Connection::CreateBasic();
  result = connection->CreateServer(Config::SERVER_PORT, server_listener);
#endif //ENABLE_SSL
  return result;
}

int main() {
  auto server_listener = std::make_shared<ServerListener>();
  auto server = CreateServer(server_listener);
  if(server) {
    log()->info("Server started at port : {}", Config::SERVER_PORT);
  } else {
    log()->error("Server failed to start at port : {}", Config::SERVER_PORT);
    return 1;
  }

  while(true) {
    sleep(1);
  }

  return 0;
}
