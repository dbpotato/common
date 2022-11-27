/*
Copyright (c) 2018 - 2022 Adam Kaniewski

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
#include "Message.h"
#include "Logger.h"
#include "Connection.h"

#include <limits>


std::atomic<uint32_t> Client::_id_counter(0);

void ClientManager::OnServerCreated(std::weak_ptr<Server> server) {
}

void ClientManager::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  DLOG(warn, "ClientManager::OnClientRead : not implemented");
}

bool ClientManager::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  DLOG(warn, "ClientManager::OnClientConnecting : not implemented");
  return true;
}

void ClientManager::OnClientConnected(std::shared_ptr<Client> client) {
  DLOG(warn, "ClientManager::OnClientConnected : not implemented");
}

void ClientManager::OnClientClosed(std::shared_ptr<Client> client) {
}

void ClientManager::OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) {
}


uint32_t Client::NextId() {
  if(_id_counter == std::numeric_limits<uint32_t>::max())
    DLOG(error, "Client's id counter overflow");
  return ++_id_counter;
}

Client::Client(int socket_fd,
               std::shared_ptr<Connection> connection,
               std::shared_ptr<SocketContext> context,
               const std::string& ip,
               int port,
               const std::string& url,
               std::weak_ptr<ClientManager> manager)
    : SocketObject(socket_fd, false, connection, context)
    , _ip(ip)
    , _port(port)
    , _url(url)
    , _manager(manager)
    , _id(NextId())
    , _is_connected(false) {
  if(auto mgr = _manager.lock()) {
    if(!mgr->IsRaw()) {
      _msg_builder.reset(new MessageBuilder());
    }
  }
}

void Client::Update(int socket, const std::string& ip) {
  _socket_fd = socket;
  _ip = ip;
}

void Client::SetManager(std::weak_ptr<ClientManager> manager) {
  _manager = manager;
}

void Client::SetMsgBuilder(std::unique_ptr<MessageBuilder> msg_builder) {
  _msg_builder = std::move(msg_builder);
}


uint32_t Client::GetId() {
  return _id;
}

const std::string& Client::GetUrl() {
  return _url;
}

const std::string& Client::GetIp() {
  return _ip;
}

int Client::GetPort() {
  return _port;
}

bool Client::Send(std::shared_ptr<Message> msg) {
  if(!IsActive()) {
    DLOG(error, "Client::Send - client is not active");
    return false;
  }
  if(!_is_connected){
    DLOG(error, "Client::Send - client is not connected");
    return false;
  }
  msg = msg->ConvertToBaseMessage();
  if(!msg) {
    DLOG(error, "Client::Send - message is not valid");
    return false;
  }
  _connection->SendMsg(SharedPtr(), msg);
  return true;
}

std::shared_ptr<Client> Client::SharedPtr() {
  return std::static_pointer_cast<Client>(shared_from_this());
}

bool Client::OnConnecting(NetError err) {
  if(auto manager = _manager.lock()) {
    return manager->OnClientConnecting(SharedPtr(), err);
  }
  return false;
}

void Client::OnConnected() {
  _is_connected = true;
  if(auto manager = _manager.lock()) {
    manager->OnClientConnected(SharedPtr());
  }
}

void Client::OnMsgWrite(std::shared_ptr<Message> msg, bool status) {
  if(auto manager = _manager.lock())
    manager->OnMsgSent(SharedPtr(), msg, status);
}

void Client::OnDataRead(Data& data) {
  if(_msg_builder) {
    std::vector<std::shared_ptr<Message> > msgs;
    _msg_builder->AddData(data, msgs);
    if(msgs.size()) {
      if(auto manager = _manager.lock()) {
        for(auto msg : msgs) {
          manager->OnClientRead(SharedPtr(), msg);
        }
      }
    }
  } else {
    auto msg = std::make_shared<Message>(data._size, data._data);
    if(auto manager = _manager.lock()) {
      manager->OnClientRead(SharedPtr(), msg);
    }
  }
}

void Client::OnConnectionClosed() {
  SocketObject::OnConnectionClosed();
  if(auto manager = _manager.lock())
    manager->OnClientClosed(SharedPtr());
}
