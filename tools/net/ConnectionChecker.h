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

#pragma once

#include "PosixThread.h"

#include <string>
#include <memory>
#include <atomic>

class Connection;
class Client;

class ConnectionKeeper {
public :
  virtual void OnConnected(std::shared_ptr<Client> client) = 0;
  virtual void OnDisconnected() = 0;
  virtual void SendPing() = 0;
};

class ConnectionChecker : public ThreadObject, public std::enable_shared_from_this<ConnectionChecker> {
public:
  enum ConnectionState{
    NOT_CONNECTED,
    CONNECTED,
    MAYBE_CONNECTED
  };
  ConnectionChecker(std::weak_ptr<ConnectionKeeper> keeper,
                    std::shared_ptr<Connection> connection,
                    size_t check_interval_in_sec,
                    const std::string& server_url,
                    int server_port);

  ~ConnectionChecker();
  void Init();
  void Wake();
  void OnThreadStarted(int thread_id) override;

protected :
  void DoConnectionCheck();
  void TryConnect();
  void SetState(ConnectionState new_state);
  std::weak_ptr<ConnectionKeeper> _keeper;
  std::shared_ptr<Connection> _connection;
  PosixThread _alive_check;
  size_t _check_interval_in_sec;
  std::string _server_url;
  int _server_port;
  std::atomic<time_t> _last_alive;
  std::atomic<ConnectionState> _state;
  std::shared_ptr<Client> _current_client;
};
