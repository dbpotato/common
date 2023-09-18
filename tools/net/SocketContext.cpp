/*
Copyright (c) 2019 - 2023 Adam Kaniewski

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

#include "SocketContext.h"
#include "Client.h"
#include "Connection.h"
#include "Logger.h"
#include "Epool.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>


SocketContext::SocketContext(SocketContext::State init_state)
    : _info(nullptr)
    , _info_next(nullptr)
    , _socket_fd(DEFAULT_SOCKET)
    , _time_passed(0.0)
    , _state(init_state)
    , _awaits_read_write(false) {

  if(_state != GETTING_INFO &&
     _state != AFTER_ACCEPTING &&
     _state != FINISHED) {
    DLOG(error, "Unexpected initialize state : {}", _state);
    _state = FAILED;
    return;
  }

  if(_state == GETTING_INFO) {
    memset(&_hints, 0, sizeof(struct addrinfo));
    _hints.ai_family = AF_INET;
    _hints.ai_socktype = SOCK_STREAM;
    _hints.ai_flags = AI_PASSIVE;
    _hints.ai_protocol = IPPROTO_TCP;
  }
}

SocketContext::~SocketContext() {
  if(_info) {
    freeaddrinfo(_info);
  }
}

NetError SocketContext::Continue(std::shared_ptr<Client> client, bool on_read_write_available) {
  if(on_read_write_available) {
    SetState(NextState());
  }

  auto start = std::chrono::system_clock::now();
  switch (_state) {
    case GETTING_INFO:
      GetAddrInfo(client);
      break;
    case CONNECTING:
      Connect(client);
      break;
    case AFTER_CONNECTING:
      AfterConnect(client);
      break;
    case AFTER_ACCEPTING:
      AfterAccept(client);
      break;
    default:
      break;
  }
  _time_passed += std::chrono::duration<double, std::milli>(std::chrono::system_clock::now() - start).count();
  TimeoutCheck();

  return StateToError();
}

void SocketContext::GetAddrInfo(std::shared_ptr<Client> client) {
  int res = getaddrinfo(client->GetUrl().c_str(),
                        std::to_string(client->GetPort()).c_str(),
                        &_hints,
                        &_info);
  if(res != 0) {
    if(res == EAI_AGAIN) {
      return;
    }
    else {
      DLOG(error, "SocketContext : getaddrinfo failed : {}", gai_strerror(res));
      SetState(FAILED);
      return;
    }
  }

  _info_next = _info;
  SetState(NextState());
}

void SocketContext::Connect(std::shared_ptr<Client> client) {
  if(_socket_fd == DEFAULT_SOCKET) {
    _socket_fd = socket(_info_next->ai_family, _info_next->ai_socktype, _info_next->ai_protocol);
    if(_socket_fd != DEFAULT_SOCKET) {
      fcntl(_socket_fd, F_SETFL, O_NONBLOCK);
    }
    else {
      DLOG(error, "SocketContext: create socket failed");
      SetState(FAILED);
      return;
    }
  }

  if (_socket_fd != DEFAULT_SOCKET) {
    int res = connect(_socket_fd, _info_next->ai_addr, _info_next->ai_addrlen);
    if(!res) {
      char ipstr[INET_ADDRSTRLEN];
      struct sockaddr_in* ipv = reinterpret_cast<struct sockaddr_in*>(_info_next->ai_addr);
      struct in_addr* addr = &(ipv->sin_addr);
      inet_ntop(_info_next->ai_family, addr, ipstr, sizeof ipstr);

      client->Update(_socket_fd, std::string(ipstr));
      Epool::GetInstance()->AddListener(client);
      SetState(NextState());
      return;
    }
    else if((errno == EINPROGRESS) || (errno ==EALREADY)) {
      return;
    }
  }

  if(_info_next->ai_next) {
    _info_next = _info_next->ai_next;
  }
  else {
    DLOG(warn, "SocketContext: connect fail : {} : {} : {}",
         client->GetUrl(),
         client->GetPort(),
         strerror(errno));

    SetState(FAILED);
    if(_socket_fd != DEFAULT_SOCKET) {
      close(_socket_fd);
      _socket_fd = DEFAULT_SOCKET;
    }
  }
}

void SocketContext::AfterConnect(std::shared_ptr<Client> client) {
  auto connection = client->GetConnection();
  NetError err = connection->AfterSocketCreated(client);
  ErrToState(err);
}

void SocketContext::AfterAccept(std::shared_ptr<Client> client) {
  auto connection = client->GetConnection();
  NetError err = connection->AfterSocketAccepted(client);
  ErrToState(err);
}

void SocketContext::ErrToState(NetError err) {
  switch(err) {
    case NetError::OK:
      SetState(NextState());
      break;
    case NetError::RETRY:
      break;
    case NetError::NEEDS_READ:
      SetState(AWAIT_READ);
      break;
    case NetError::NEEDS_WRITE:
      SetState(AWAIT_WRITE);
      break;
    case NetError::TIMEOUT:
      SetState(TIMEOUT);
      break;
    case NetError::FAILED:
      SetState(FAILED);
      break;
    default:
      SetState(FAILED);
  }
}

NetError SocketContext::StateToError() {
  switch(_state) {
    case TIMEOUT:
      return NetError::TIMEOUT;
    case FAILED:
      return NetError::FAILED;
    case FINISHED :
      return NetError::OK;
    case AWAIT_READ:
      return NetError::NEEDS_READ;
    case AWAIT_WRITE:
      return NetError::NEEDS_WRITE;
    default:
      return NetError::RETRY;
  }
}

SocketContext::State SocketContext::NextState() {
  switch(_state) {
    case GETTING_INFO:
      return CONNECTING;
    case CONNECTING:
      return AFTER_CONNECTING;
    case AFTER_CONNECTING:
    case AFTER_ACCEPTING:
      return FINISHED;
    case AWAIT_READ:
    case AWAIT_WRITE:
      return _previous_state;
    default:
      return _state;
  }
}

void SocketContext::TimeoutCheck() {
  if(_time_passed < CONNECT_TIMEOUT_IN_MS)
    return;

  if((_state == FINISHED) ||
     (_state == FAILED))
    return ;

  SetState(TIMEOUT);
}

void SocketContext::SetState(State state) {
  _previous_state = _state;
  _state = state;
}

bool SocketContext::AwaitsConnectingReadWrite() {
  return ((_state == AWAIT_READ) ||
          (_state == AWAIT_WRITE));
}
