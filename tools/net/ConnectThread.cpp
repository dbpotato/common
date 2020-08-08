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

#include "ConnectThread.h"
#include "Transporter.h"
#include "Client.h"
#include "SocketContext.h"

#include <vector>

std::weak_ptr<ConnectThread> ConnectThread::_instance;

std::shared_ptr<ConnectThread> ConnectThread::GetInstance() {
  std::shared_ptr<ConnectThread> instance;
  if(!(instance =_instance.lock())) {
    instance.reset(new ConnectThread());
    instance->Init();
    _instance = instance;
  }
  return instance;
}

ConnectThread::ConnectThread() : _is_ready(false) {
}

ConnectThread::~ConnectThread() {
  _run_thread.Stop();
  _run_thread.Join();
}

void ConnectThread::Init() {
  _run_thread.Run(shared_from_this(), 0);
}

void ConnectThread::WaitOnCondition() {
  std::unique_lock<std::mutex> lock(_condition_mutex);
  _condition.wait(lock, [this]{return _is_ready;});
  _is_ready = false;
  lock.unlock();
}

void ConnectThread::Notify() {
  {
    std::lock_guard<std::mutex> lock(_condition_mutex);
    _is_ready = true;
  }
  _condition.notify_one();
}

void ConnectThread::OnThreadStarted(int thread_id) {
  while(_run_thread.ShouldRun()) {
    WaitOnCondition();
    std::vector<std::shared_ptr<Client>> clients;

    if(!_run_thread.ShouldRun())
      return;
    ConnectClients();
  }
}

void ConnectThread::AddClient(std::shared_ptr<Client> client) {
  _clients.Add(client);
  Notify();
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
  if(err == NetError::OK) {
    transporter->AddSocket(client);
  }
  client->OnConnected(err);
  if(client->IsActive())
    transporter->EnableSocket(client);
}
