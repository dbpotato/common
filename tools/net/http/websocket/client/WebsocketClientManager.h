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

#pragma once

#include "Client.h"

#include <memory>
#include <mutex>
#include <string>


class Connection;
class HttpMessage;
class Message;
class WebsocketMessage;


class WebsocketClientManager : public ClientManager
                             , public std::enable_shared_from_this<WebsocketClientManager>  {
public:
  WebsocketClientManager();
  virtual ~WebsocketClientManager();
  void CreateClient(std::shared_ptr<Connection> connection,
                    const std::string& host,
                    int port,
                    const std::string& request_target = "/");
protected:
  class InternalWebsocketClientManager : public ClientManager
                                       , public std::enable_shared_from_this<InternalWebsocketClientManager>  {
  public:
    InternalWebsocketClientManager(std::weak_ptr<ClientManager> target_manager,
                                  const std::string& host,
                                  int port,
                                  const std::string& request_target);
    void Init();
    bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
    void OnClientConnected(std::shared_ptr<Client> client) override;
    void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
    void OnClientClosed(std::shared_ptr<Client> client) override;
  protected:
    void SendUpgradeRequest(std::shared_ptr<Client> client);
    bool HandleUpgradeResponse(std::shared_ptr<Client> client, std::shared_ptr<HttpMessage> msg);
    void OnFailed();
    void Lock();
    void Release();
    std::string GenerateClientKey();
    std::string ExpectedAcceptHash(const std::string& key);
    std::weak_ptr<ClientManager> _target_manager;
    std::string _client_key;
    std::shared_ptr<InternalWebsocketClientManager> _self;
    std::string _host;
    int _port;
    std::string _request_target;
    std::shared_ptr<Client> _connecting_client;
  };
  bool MaybeHandleWebsocketMessage(std::shared_ptr<Client> client, std::shared_ptr<WebsocketMessage> msg);
};
