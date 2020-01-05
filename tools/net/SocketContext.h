/*
Copyright (c) 2019 - 2020 Adam Kaniewski

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

#include <string>
#include <memory>
#include <netdb.h>

#include "Utils.h"


class Client;
class Connection;

class SocketContext {
public:
  enum State {
    GETTING_INFO,
    CONNECTING,
    AFTER_CONNECTING,
    AFTER_ACCEPTING,
    FINISHED,
    TIMEOUT,
    FAILED,
  };

  SocketContext(bool from_accept);
  ~SocketContext();
  NetError Continue(std::shared_ptr<Client> client,
                    std::shared_ptr<Connection> connection);
  virtual bool HasReadPending() {return false;}
private:
  void GetAddrInfo(std::shared_ptr<Client> client);
  void Connect(std::shared_ptr<Client> client);
  void AfterConnect(std::shared_ptr<Client> client,
                    std::shared_ptr<Connection> connection);
  void AfterAccept(std::shared_ptr<Client> client,
                   std::shared_ptr<Connection> connection);
  void SetState(State);
  void ErrToState(NetError err);
  NetError StateToError();
  State NextState();
  void TimeoutCheck();

  addrinfo _hints;
  addrinfo* _info;
  addrinfo* _info_next;
  int _socket_handle;
  double _time_passed;
  State _state;
  std::string _ip;
};

