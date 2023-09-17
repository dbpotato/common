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

#include <memory>
#include <vector>

#include "Client.h"


class Message;
class Proxy;

class ProxyChannel : public std::enable_shared_from_this<ProxyChannel>
                   , public ClientManager {
public:
  ProxyChannel(size_t id,
              std::shared_ptr<Client> src_client,
              std::shared_ptr<Proxy> proxy);

  void Init();
  size_t GetId();
private:
  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  void OnClientConnected(std::shared_ptr<Client> client) override;
  void OnClientClosed(std::shared_ptr<Client> client) override;
  void OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) override;

  size_t _id;
  std::shared_ptr<Client> _dest_client;
  std::shared_ptr<Client> _src_client;
  std::shared_ptr<Proxy> _proxy;
  std::vector<std::shared_ptr<Message>> _pending_msgs;
};
