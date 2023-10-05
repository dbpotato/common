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

#include "Client.h"
#include "Config.h"
#include "Connection.h"
#include "Logger.h"
#include "Data.h"
#include "Message.h"
#include "NetUtils.h"

#ifdef ENABLE_SSL
  #include "ConnectionMTls.h"
  #include "MtlsCppWrapper.h"

  using namespace MtlsCppWrapper;
#endif //ENABLE_SSL


class ClientHandler : public ClientManager
                    , public std::enable_shared_from_this<ClientHandler> {
public :
 void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
 bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
 void OnClientConnected(std::shared_ptr<Client> client) override;
 void OnClientClosed(std::shared_ptr<Client> client) override;
private:
 bool IsClientVerified(std::shared_ptr<Client> client);
 std::shared_ptr<Client> _client;
};

void ClientHandler::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  log()->info("Client Read : {}", msg->GetData()->ToString());
}

bool ClientHandler::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  bool verified = IsClientVerified(client);
  log()->info("Client connecting : err : {}, ssl verified : {}",
              (int)err,
              verified);
  return true;
}

void ClientHandler::OnClientConnected(std::shared_ptr<Client> client) {
  _client = client;
  log()->info("Client connected, Sending message : {}", Config::MSG_FOR_SERVER);
  _client->Send(std::make_shared<Message>(Config::MSG_FOR_SERVER));
}

void ClientHandler::OnClientClosed(std::shared_ptr<Client> client) {
  log()->info("Client closed");
  _client = nullptr;
}

bool ClientHandler::IsClientVerified(std::shared_ptr<Client> client) {
  bool result = false;
#ifdef ENABLE_SSL
  auto context = std::static_pointer_cast<SocketContextMtls>(client->GetContext());
  result = context->IsSslVerified();
#endif //ENABLE_SSL
  return result;
}

void CreateClient(std::shared_ptr<ClientHandler> handler) {
#ifdef ENABLE_SSL
  auto config = MtlsCppConfig::CreateClientConfig(Config::CLIENT_KEY, Config::CLIENT_CERT, Config::AUTH_CERT);
  auto connection = ConnectionMTls::CreateMTls(config);
#else
  auto connection = Connection::CreateBasic();
#endif //ENABLE_SSL
  connection->CreateClient(Config::SERVER_PORT,
                           Config::SERVER_URL,
                           handler);
}

int main() {
  auto client_handler = std::make_shared<ClientHandler>();
  CreateClient(client_handler);

  while(true) {
    sleep(1);
  }

  return 0;
}
