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

#include "ProxyChannel.h"
#include "Proxy.h"
#include "Logger.h"


ProxyChannel::ProxyChannel(size_t id
                          ,std::shared_ptr<Client> src_client
                          ,std::shared_ptr<Proxy> proxy)
    : _id(id)
    , _src_client(src_client)
    , _proxy(proxy) {
}

void ProxyChannel::Init() {
  _src_client->SetManager(shared_from_this());
}

size_t ProxyChannel::GetId() {
  return _id;
}

void ProxyChannel::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  if(client == _src_client) {
    auto processed_msg = _proxy->OnClientMessageRead(msg);
    if(_dest_client) {
      _dest_client->Send(processed_msg);
    } else {
      _pending_msgs.emplace_back(processed_msg);
      _proxy->CreateHostClient(shared_from_this());
    }
  } else {
    auto processed_msg = _proxy->OnHostMessageRead(msg);
    _src_client->Send(processed_msg);
  }
}

bool ProxyChannel::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  if(err != NetError::OK) {
    DLOG(warn, "ProxyChannel : failed to connect to host target");
    _proxy->OnChannelClosed(shared_from_this());
    return false;
  } else {
    auto msg_builder = _proxy->CreateHostMessageBuilder();
    client->SetMsgBuilder(std::move(msg_builder));
    return true;
  }
  return false;
}

void ProxyChannel::OnClientConnected(std::shared_ptr<Client> client) {
  if(client == _src_client) {
    return;
  }
  _dest_client = client;
  for(auto msg : _pending_msgs) {
    _dest_client->Send(msg);
  }
  _pending_msgs.clear();
}

void ProxyChannel::OnClientClosed(std::shared_ptr<Client> client) {
  _proxy->OnChannelClosed(shared_from_this());
}

void ProxyChannel::OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) {
  if(!success) {
    _proxy->OnChannelClosed(shared_from_this());
  }
}