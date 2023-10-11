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

#pragma once

#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <vector>

#include "Client.h"
#include "Server.h"
#include "Connection.h"
#include "Message.h"

class ProxyChannel;

class Proxy : public std::enable_shared_from_this<Proxy>
            , public ClientManager {
public:
  Proxy(int listen_port, std::string host_url, int host_port);
  bool Init();
  virtual std::unique_ptr<MessageBuilder> CreateHostMessageBuilder();
  virtual void CreateHostClient(std::shared_ptr<ProxyChannel> channel,
                              std::shared_ptr<Client> client,
                              std::shared_ptr<Message> msg);
  virtual std::shared_ptr<Message> OnClientMessageRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg);
  virtual std::shared_ptr<Message> OnHostMessageRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg);
  void OnChannelClosed(std::shared_ptr<ProxyChannel>);


//ClientManager
  virtual bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  void OnClientConnected(std::shared_ptr<Client> client) override;
  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;

protected:
  virtual std::shared_ptr<Connection> CreateHostConnection();
  virtual std::shared_ptr<Connection> CreateClientsConnection();
  virtual std::shared_ptr<Server> CreateServer();
  virtual std::unique_ptr<MessageBuilder> CreateClientMessageBuilder();
  std::shared_ptr<Connection> _host_connection;
  std::shared_ptr<Connection> _clients_connection;
private:
  size_t _channel_id_counter;
  int _listen_port;
  std::string _host_url;
  int _host_port;
  std::map<size_t, std::shared_ptr<ProxyChannel> > _channels;
  std::shared_ptr<Server> _server;
  std::mutex _channel_mutex;
};
