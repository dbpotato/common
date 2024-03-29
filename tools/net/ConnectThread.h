/*
Copyright (c) 2020 - 2023 Adam Kaniewski

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

#include "NetUtils.h"

#include <map>
#include <memory>


class Client;
class ThreadLoop;
class Epool;


class ConnectThread : public std::enable_shared_from_this<ConnectThread> {

public:
  static std::shared_ptr<ConnectThread> GetInstance();
  void AddClient(std::shared_ptr<Client> client);
  void Continue(std::shared_ptr<Client> client);

private:
  ConnectThread();
  void ConnectClients();
  void OnConnectComplete(std::shared_ptr<Client>, NetError err);

  static std::weak_ptr<ConnectThread> _instance;
  std::map<uint32_t, std::shared_ptr<Client>> _clients;
  std::shared_ptr<ThreadLoop> _thread_loop;
  std::shared_ptr<Epool> _epool;
};
