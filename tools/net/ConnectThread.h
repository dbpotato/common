/*
Copyright (c) 2020 Adam Kaniewski

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

#include "PosixThread.h"
#include "Collector.h"
#include "Utils.h"

#include <memory>
#include <mutex>
#include <condition_variable>


class Client;

class ConnectThread : public std::enable_shared_from_this<ConnectThread>
                    , public ThreadObject {
public:
  static std::shared_ptr<ConnectThread> GetInstance();
  ~ConnectThread();
  void AddClient(std::shared_ptr<Client> client);
  void OnThreadStarted(int thread_id) override;

private:
  ConnectThread();
  void Init();
  void WaitOnCondition();
  void Notify();
  void ConnectClients();
  void OnConnectComplete(std::shared_ptr<Client>, NetError err);

  static std::weak_ptr<ConnectThread> _instance;

  PosixThread _run_thread;
  Collector<std::shared_ptr<Client>> _clients;
  std::condition_variable _condition;
  std::mutex _condition_mutex;
  bool _is_ready;
};
