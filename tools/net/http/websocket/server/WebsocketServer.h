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

#include "Client.h"
#include "HttpServer.h"

#include <memory>
#include <string>


class Server;
class HttpHeader;
class WebsocketMessage;
class WebsocketServer;

class WebsocketClientManager : public ClientManager {
public:
  WebsocketClientManager(std::shared_ptr<WebsocketServer> owner);
  bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  void OnClientConnected(std::shared_ptr<Client> client) override;
  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  void OnClientClosed(std::shared_ptr<Client> client) override;
  bool IsRaw() override;
private:
  std::shared_ptr<WebsocketServer> _owner;
};

class WebsocketClientListener {
public :
  virtual bool OnWsClientConnected(std::shared_ptr<Client> client, const std::string& request_arg) = 0;
  virtual void OnWsClientMessage(std::shared_ptr<Client> client, std::shared_ptr<WebsocketMessage> message) = 0;
  virtual void OnWsClientClosed(std::shared_ptr<Client> client) = 0;
};

class WebsocketServer : public HttpServer {
public:
  WebsocketServer();
  bool Init(std::shared_ptr<Connection> connection,
            std::shared_ptr<HttpRequestHandler> request_handler,
            std::shared_ptr<WebsocketClientListener> ws_client_listener,
            int port);
  friend void WebsocketClientManager::OnClientRead(std::shared_ptr<Client>, std::shared_ptr<Message>);

private:
  void ProcessRequest(std::shared_ptr<Client> client, std::shared_ptr<HttpMessage> msg) override;
  bool CheckForProtocolUpgradeRequest(std::shared_ptr<Client> client, std::shared_ptr<HttpHeader> header);

  std::string PrepareWebSocketAccept(const std::string& key);
  void SendHandshakeResponse(std::shared_ptr<Client> client, const std::string& accept_hash);
  void SendPongMsg(std::shared_ptr<Client> client, std::shared_ptr<WebsocketMessage> websocket_msg);

  void OnWsClose(std::shared_ptr<Client> client);
  void OnWsPing(std::shared_ptr<Client> client, std::shared_ptr<WebsocketMessage> msg);
  void OnWsPong(std::shared_ptr<Client> client);
  void OnWsMessage(std::shared_ptr<Client> client, std::shared_ptr<WebsocketMessage> msg);

  std::shared_ptr<WebsocketClientManager> _ws_client_manager;
  std::shared_ptr<WebsocketClientListener> _ws_client_listener;
};
