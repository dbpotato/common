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

#include "Proxy.h"
#include "ProxyChannel.h"
#include "Logger.h"
#include "Connection.h"


Proxy::Proxy(int listen_port, std::string host_url, int host_port)
    : _channel_id_counter(0)
    , _listen_port(listen_port)
    , _host_url(host_url)
    , _host_port(host_port) {
}

bool Proxy::Init() {
  _clients_connection = CreateClientsConnection();
  _host_connection = CreateHostConnection();
  _server = CreateServer();
  return (_server != nullptr);
}

std::shared_ptr<Connection> Proxy::CreateClientsConnection() {
  return Connection::CreateBasic();
}

std::shared_ptr<Connection> Proxy::CreateHostConnection() {
  return _clients_connection;
}

std::shared_ptr<Server> Proxy::CreateServer() {
  return _clients_connection->CreateServer(_listen_port, shared_from_this());
}

std::unique_ptr<MessageBuilder> Proxy::CreateHostMessageBuilder() {
  return nullptr;
}

std::unique_ptr<MessageBuilder> Proxy::CreateClientMessageBuilder() {
  return nullptr;
}

void Proxy::CreateHostClient(std::shared_ptr<ProxyChannel> channel) {
  _host_connection->CreateClient(_host_port, _host_url, channel);
}

std::shared_ptr<Message> Proxy::OnClientMessageRead(std::shared_ptr<Message> msg) {
  return msg;
}

std::shared_ptr<Message> Proxy::OnHostMessageRead(std::shared_ptr<Message> msg) {
  return msg;
}

bool Proxy::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  if(err != NetError::OK) {
    return false;
  }

  auto msg_builder = CreateClientMessageBuilder();
  client->SetMsgBuilder(std::move(msg_builder));

  std::shared_ptr<ProxyChannel> pc = std::make_shared<ProxyChannel>(_channel_id_counter
                                                                    ,client
                                                                    ,shared_from_this());
  _channel_mutex.lock();
  _channels.insert(std::make_pair(_channel_id_counter,pc));
  _channel_id_counter++;
  _channel_mutex.unlock();
  pc->Init();

  return true;
}

void Proxy::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  //Not reachable
}

void Proxy::OnClientConnected(std::shared_ptr<Client> src_client) {
  //Not reachable
}

void Proxy::OnChannelClosed(std::shared_ptr<ProxyChannel> channel) {
  _channel_mutex.lock();
  auto it = _channels.find(channel->GetId());
  if(it != _channels.end()) {
    _channels.erase(it);
  } else {
    DLOG(error, "Proxy : failed to find closed channel");
  }
  _channel_mutex.unlock();
}
