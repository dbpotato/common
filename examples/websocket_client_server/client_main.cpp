/*
Copyright (c) 2026 Adam Kaniewski

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
#include "DataResource.h"
#include "Logger.h"
#include "WebsocketClientManager.h"
#include "WebsocketMessage.h"

#include <unistd.h>


const static std::string SERVER_HOST = "127.0.0.1";
const static int SERVER_PORT = 8080;


class ClientManagerImpl
    : public WebsocketClientManager {
public:
  bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  void OnClientConnected(std::shared_ptr<Client> client) override;
  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  void OnClientClosed(std::shared_ptr<Client> client) override;
private :
  std::shared_ptr<Client> _client;
};

void ClientManagerImpl::OnClientConnected(std::shared_ptr<Client>  client) {
  log()->info("WS Client connected, sending : Hello");
  _client = client;
  _client->Send(std::make_shared<WebsocketMessage>("Hello", true));
}


bool ClientManagerImpl::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  log()->info("WS Client Connecting : err {}", (int)err);
  return (err == NetError::OK);
}

void ClientManagerImpl::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  log()->info("WS Client Read");
  auto ws_msg = std::dynamic_pointer_cast<WebsocketMessage>(msg);
  if(MaybeHandleWebsocketMessage(client, ws_msg)) {
    return;
  }

  auto data = ws_msg->GetResource()->GetData();
  log()->info("WS Client got message : {}", data ? data->ToString() : std::string("<empty>"));
}

void ClientManagerImpl::OnClientClosed(std::shared_ptr<Client> client) {
   log()->info("WS Client close");
}

int main() {
  auto connection = Connection::CreateBasic();
  auto handler = std::make_shared<ClientManagerImpl>();
  handler->CreateClient(connection, SERVER_HOST, SERVER_PORT);

  log()->info("WebsocketClient connecting to {}:{}", SERVER_HOST, SERVER_PORT);

  while(true) {
    sleep(1);
  }

  return 0;
}
