/*
Copyright (c) 2018 Adam Kaniewski

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

#include "ServerImpl.h"

ServerImpl::ServerImpl() : _is_raw(false){
}

bool ServerImpl::Init(int port, std::shared_ptr<Connection> connection, bool is_raw) {
  if(!_server) {
    _is_raw = is_raw;
    _connection = connection;
    _server = _connection->CreateServer(port);
    if(!_server)
      return false;
    else {
      _server->Init(shared_from_this());
      return true;
    }
  }
  return false;
}


void ServerImpl::AddListener(std::weak_ptr<ServerListener> listener) {
  _listeners.push_back(listener);
}

void ServerImpl::OnClientConnected(std::shared_ptr<Client> client) {
  AddClient(client);
  for(auto listener_wp : _listeners)
    if(auto listener = listener_wp.lock())
       listener->OnClientConnected(client);
  client->Start(shared_from_this(), _is_raw);
}

void ServerImpl::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  for(auto listener_wp : _listeners)
    if(auto listener = listener_wp.lock())
       listener->OnClientRead(client, msg);
}

void ServerImpl::OnClientClosed(std::shared_ptr<Client> client) {
  RemoveClient(client->GetId());
  for(auto listener_wp : _listeners)
    if(auto listener = listener_wp.lock())
       listener->OnClientClosed(client);
}

void ServerImpl::OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) {
  for(auto listener_wp : _listeners)
    if(auto listener = listener_wp.lock())
       listener->OnMsgSent(client, msg, success);
}

void ServerImpl::AddClient(std::shared_ptr<Client> client) {
  std::lock_guard<std::mutex> lock(_mutex);
  _clients.insert(std::make_pair(client->GetId(),client));
}

bool ServerImpl::RemoveClient(uint32_t id) {
  std::lock_guard<std::mutex> lock(_mutex);
  auto it = _clients.find(id);
  if(it!=_clients.end()) {
    _clients.erase(it);
    return true;
  }
  return false;
}

std::shared_ptr<Client> ServerImpl::GetClient(uint32_t id) {
  std::lock_guard<std::mutex> lock(_mutex);
  std::shared_ptr<Client> result;
  auto it = _clients.find(id);
  if(it!=_clients.end())
    result = it->second;

  return result;
}

void ServerImpl::GetClients(std::vector<std::shared_ptr<Client> >& vec) {
  std::lock_guard<std::mutex> lock(_mutex);
  for(auto kv : _clients)
    vec.push_back(kv.second);
}

void ServerImpl::Clear() {
  std::lock_guard<std::mutex> lock(_mutex);
  _clients.clear();
}

