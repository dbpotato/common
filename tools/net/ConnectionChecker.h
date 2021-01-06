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
#include "Client.h"

#include <string>
#include <memory>
#include <atomic>

class Connection;
class Client;
class ClientManager;


class ConnectionChecker : public ThreadObject
                        , public ClientManager
                        , public std::enable_shared_from_this<ConnectionChecker> {
public:
  enum ConnectionState{
    NOT_CONNECTED,
    CONNECTED,
    MAYBE_CONNECTED
  };
  ConnectionChecker(std::shared_ptr<Connection> connection,
                    size_t check_interval_in_sec,
                    const std::string& server_url,
                    int server_port,
                    bool is_raw);

  ~ConnectionChecker();
  void Init();
  void OnThreadStarted(int thread_id) override;

  virtual void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  virtual bool OnClientConnected(std::shared_ptr<Client> client, NetError err) override;
  virtual void OnClientClosed(std::shared_ptr<Client> client) override;
  bool IsRaw() override;

  virtual std::shared_ptr<Message> CreatePingMessage() = 0;

protected :
  std::shared_ptr<Client> GetClient();

private :
  void TryConnect();
  void SendPing();
  void SetState(ConnectionState new_state);
  std::shared_ptr<Connection> _connection;
  std::shared_ptr<Client> _current_client;
  PosixThread _alive_check;
  size_t _check_interval_in_sec;
  std::string _server_url;
  int _server_port;
  std::atomic<ConnectionState> _state;
  bool _is_raw;
};
