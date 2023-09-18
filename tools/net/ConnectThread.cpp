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

#include "ConnectThread.h"
#include "Epool.h"
#include "Client.h"
#include "SocketContext.h"
#include "ThreadLoop.h"


std::weak_ptr<ConnectThread> ConnectThread::_instance;

std::shared_ptr<ConnectThread> ConnectThread::GetInstance() {
  std::shared_ptr<ConnectThread> instance;
  if(!(instance =_instance.lock())) {
    instance.reset(new ConnectThread());
    _instance = instance;
  }
  return instance;
}

ConnectThread::ConnectThread() {
  _thread_loop = std::make_shared<ThreadLoop>();
  _thread_loop->Init();
  _epool = Epool::GetInstance();
}

void ConnectThread::AddClient(std::shared_ptr<Client> client) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&ConnectThread::AddClient, shared_from_this(), client));
    return;
  }
  auto pair = std::make_pair<uint32_t, std::shared_ptr<Client>>(client->GetId(), std::move(client));
  _clients.insert(pair);
  ConnectClients();
}

void ConnectThread::Continue(std::shared_ptr<Client> client) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&ConnectThread::Continue, shared_from_this(), client));
    return;
  }

  auto context = client->GetContext();
  NetError err = context->Continue(client, true);
  if((err == NetError::NEEDS_READ) || (err == NetError::NEEDS_WRITE)) {
    if(err == NetError::NEEDS_READ) {
      _epool->SetListenerAwaitingRead(client, true);
    } else {
      _epool->SetListenerAwaitingWrite(client, true);
    }
  } else if (err != NetError::RETRY) {
    OnConnectComplete(client, err);
    _clients.erase(client->GetId());
  }
}

void ConnectThread::ConnectClients() {
  bool run_again = false;
  auto it = _clients.begin();
  while(it != _clients.end()) {
    auto client = it->second;
    auto context = client->GetContext();

    if(context->AwaitsConnectingReadWrite()) {
      ++it;
      continue;
    }

    NetError err = context->Continue(client);

    if(err == NetError::RETRY) {
      run_again = true;
      ++it;
    } else if((err == NetError::NEEDS_READ) || (err == NetError::NEEDS_WRITE)) {
      if(err == NetError::NEEDS_READ) {
        _epool->SetListenerAwaitingRead(client, true);
      } else {
        _epool->SetListenerAwaitingWrite(client, true);
      }
      ++it;
    } else {
      OnConnectComplete(client, err);
      it = _clients.erase(it);
    }
  }

  if(run_again) {
    _thread_loop->Post(std::bind(&ConnectThread::ConnectClients, shared_from_this()));
  }
}

void ConnectThread::OnConnectComplete(std::shared_ptr<Client> client, NetError err) {
  bool client_accept = client->OnConnecting(err);
  if((err == NetError::OK) && client_accept) {
    if(client->IsActive()) {
      _epool->SetListenerAwaitingRead(client, true);
    }
    client->OnConnected();
  }
}
