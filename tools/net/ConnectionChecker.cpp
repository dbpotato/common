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

#include "ConnectionChecker.h"
#include "Connection.h"
#include "Logger.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctime>
#include <chrono>
#include <thread>

ConnectionChecker::ConnectionChecker(std::weak_ptr<ConnectionKeeper> keeper,
                  std::shared_ptr<Connection> connection,
                  size_t check_interval_in_sec,
                  const std::string& server_url,
                  int server_port)
  : _keeper(keeper)
  , _connection(connection)
  , _check_interval_in_sec(check_interval_in_sec)
  , _server_url(server_url)
  , _server_port(server_port)
  , _state(NOT_CONNECTED) {
}

ConnectionChecker::~ConnectionChecker() {
  _alive_check.Stop();
  _alive_check.Join();
}

void ConnectionChecker::Init() {
  _alive_check.Run(shared_from_this());
}

void ConnectionChecker::Reset() {
  DLOG(info, "ConnectionChecker: Reset");
  _current_client.reset();
  _state = NOT_CONNECTED;
}

void ConnectionChecker::SetState(ConnectionState new_state) {
  if(_state != new_state) {
    if(auto keeper = _keeper.lock()) {
      switch (new_state) {
        case NOT_CONNECTED:
          keeper->OnDisconnected();
          DLOG(info, "ConnectStatus: NOT_CONNECTED");
          break;
        case CONNECTED:
          if(_state == NOT_CONNECTED)
            keeper->OnConnected(_current_client);
          DLOG(info, "ConnectStatus: CONNECTED");
          break;
        case MAYBE_CONNECTED:
          keeper->SendPing();
          DLOG(info, "ConnectStatus: MAYBE_CONNECTED");
          break;
        default:
          break;
      }
    }
    _state = new_state;
  }
}

void ConnectionChecker::OnThreadStarted(int id){
  while(_alive_check.ShouldRun()) {
    switch (_state) {
      case NOT_CONNECTED:
      case MAYBE_CONNECTED:
        TryConnect();
        break;
      case CONNECTED:
        SetState(MAYBE_CONNECTED);
        break;
      default:
        break;
    }
    std::this_thread::sleep_for(std::chrono::seconds(_check_interval_in_sec));
  }
}

void ConnectionChecker::TryConnect() {
  _current_client = _connection->CreateClient(_server_port, _server_url);
  if(_current_client) {
    SetState(CONNECTED);
  }
  else {
    SetState(NOT_CONNECTED);
  }
}

void ConnectionChecker::Wake() {
  SetState(CONNECTED);
}
