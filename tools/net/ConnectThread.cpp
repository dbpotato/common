/*
Copyright (c) 2020 - 2021 Adam Kaniewski

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
#include "Transporter.h"
#include "Client.h"
#include "SocketContext.h"
#include "ThreadLoop.h"

#include <vector>

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
}

void ConnectThread::AddClient(std::shared_ptr<Client> client) {
  _clients.Add(client);
  _thread_loop->Post(std::bind(&ConnectThread::ConnectClients, shared_from_this()));
}

void ConnectThread::ConnectClients() {
  std::vector<std::shared_ptr<Client>> clients_vec;
  _clients.Collect(clients_vec);

  do {
    auto it = clients_vec.begin();
    while(it != clients_vec.end()) {
      std::shared_ptr<Client> client = *it;
      NetError err = client->GetContext()->Continue(client);
      if(err != NetError::RETRY) {
        OnConnectComplete(client, err);
        it = clients_vec.erase(it);
      }
      else {
        ++it;
      }
    }
    std::vector<std::shared_ptr<Client>> new_clients;
    _clients.Collect(new_clients);
    clients_vec.insert(clients_vec.end(), new_clients.begin(), new_clients.end());
  } while (clients_vec.size());
}

void ConnectThread::OnConnectComplete(std::shared_ptr<Client> client, NetError err) {
  auto transporter = Transporter::GetInstance();

  if(client->IsValid()) {
    transporter->AddSocket(client);
  }

  if( (client->OnConnecting(err)) &&
      (err == NetError::OK) &&
      (client->IsActive()) ) {
    transporter->EnableSocket(client);
    client->OnConnected();
  }
}
