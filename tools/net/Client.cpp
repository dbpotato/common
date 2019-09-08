/*
Copyright (c) 2018 - 2019 Adam Kaniewski

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

uint32_t Client::NextId() {
  if(_id_counter == std::numeric_limits<uint32_t>::max())
    DLOG(error, "Client's id counter overflow");
  return ++_id_counter;
}

Client::Client(size_t raw_handle,
               std::shared_ptr<Connection> connection,
               const std::string& ip,
               int port,
               const std::string& url)
    : SocketObject(raw_handle, false, connection)
    , _id(NextId())
    , _is_raw(false)
    , _is_started(false)
    , _url(url)
    , _ip(ip)
    , _port(port) {
}

int Client::GetId() {
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

void Client::Send(std::shared_ptr<Message> msg) {
  _connection->SendMsg(shared_from_this(), msg);
}

void Client::Start(std::weak_ptr<ClientManager> mgr, bool is_raw) {
  if(!_is_started) {
    _manager = mgr;
    _is_raw = is_raw;
    _is_started = true;
  }
  else
    DLOG(error, "Client::Start - already started,id : {}", _id);
}

std::shared_ptr<Client> Client::SharedPtr() {
  return std::static_pointer_cast<Client>(shared_from_this());
}

void Client::OnMsgWrite(std::shared_ptr<Message> msg, bool status) {
  if(auto manager = _manager.lock())
    manager->OnMsgSent(SharedPtr(), msg, status);
}

void Client::OnDataRead(Data& data) {
  if(!_is_raw) {
    std::vector<std::shared_ptr<Message> > msgs;
    _msg_builder.AddData(data, msgs);
    if(msgs.size())
    if(auto manager = _manager.lock())
      for(auto msg : msgs)
        manager->OnClientRead(SharedPtr(), msg);
  }
  else {
    auto msg = std::make_shared<Message>(0, data._size, data._data);
    msg->_is_raw = true;
    if(auto manager = _manager.lock())
      manager->OnClientRead(SharedPtr(), msg);
  }
}

void Client::OnConnectionClosed() {
  if(auto manager = _manager.lock())
    manager->OnClientClosed(SharedPtr());
}

bool Client::IsActive() {
  return _is_started;
}
