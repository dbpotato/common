/*
Copyright (c) 2018 - 2020 Adam Kaniewski

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

#include "Server.h"


Server::Server(int raw_handle,
               std::shared_ptr<Connection> connection,
               std::vector<std::weak_ptr<ClientManager> >& listeners,
               bool is_raw)
    : SocketObject(raw_handle, true, connection)
    ,_listeners(listeners)
    ,_is_raw(is_raw) {
}

void Server::Init() {
  auto sptr = std::static_pointer_cast<Server>(shared_from_this());
  for(auto listener_wp : _listeners)
    if(auto listener = listener_wp.lock())
       listener->OnServerCreated(sptr);
}

void Server::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  for(auto listener_wp : _listeners)
    if(auto listener = listener_wp.lock())
       listener->OnClientRead(client, msg);
}

void Server::OnClientClosed(std::shared_ptr<Client> client) {
  RemoveClient(client->GetId());
  for(auto listener_wp : _listeners)
    if(auto listener = listener_wp.lock())
       listener->OnClientClosed(client);
}

void Server::OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) {
  for(auto listener_wp : _listeners)
    if(auto listener = listener_wp.lock())
       listener->OnMsgSent(client, msg, success);
}

void Server::OnClientConnected(std::shared_ptr<Client> client, NetError err) {
  if(err == NetError::OK) {
    AddClient(client);
  }

  for(auto listener_wp : _listeners)
    if(auto listener = listener_wp.lock())
      listener->OnClientConnected(client, err);
}

bool Server::IsRaw() {
  return _is_raw;
}

void Server::AddClient(std::shared_ptr<Client> client) {
  std::lock_guard<std::mutex> lock(_mutex);
  _clients.insert(std::make_pair(client->GetId(),client));
}

bool Server::RemoveClient(uint32_t id) {
  std::lock_guard<std::mutex> lock(_mutex);
  auto it = _clients.find(id);
  if(it!=_clients.end()) {
    _clients.erase(it);
    return true;
  }
  return false;
}

std::shared_ptr<Client> Server::GetClient(uint32_t id) {
  std::lock_guard<std::mutex> lock(_mutex);
  std::shared_ptr<Client> result;
  auto it = _clients.find(id);
  if(it!=_clients.end())
    result = it->second;

  return result;
}

void Server::GetClients(std::vector<std::shared_ptr<Client> >& vec) {
  std::lock_guard<std::mutex> lock(_mutex);
  for(auto kv : _clients)
    vec.push_back(kv.second);
}

void Server::Clear() {
  std::lock_guard<std::mutex> lock(_mutex);
  _clients.clear();
}

