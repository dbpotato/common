/*
Copyright (c) 2019 Adam Kaniewski

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

#include "Transporter.h"
#include "Connection.h"
#include "Message.h"
#include "SocketObject.h"
#include "Client.h"
#include "Logger.h"

std::weak_ptr<Transporter> Transporter::_instance;

Transporter::Transporter() {
}

Transporter::~Transporter() {
  _run_thread.Stop();
  _run_thread.Join();
}

void Transporter::Init() {
  _run_thread.Run(shared_from_this(), 0);
}

void Transporter::OnThreadStarted(int thread_id) {
  while(_run_thread.ShouldRun()) {
    CollectActiveSockets();
    std::this_thread::sleep_for(std::chrono::milliseconds(TRANSPORTER_SLEEP_IN_US));
  }
}

std::shared_ptr<Transporter> Transporter::GetTransporter(std::weak_ptr<Connection> conn) {
  std::shared_ptr<Transporter> instance;
  if(!(instance =_instance.lock())) {
    instance.reset(new Transporter());
    instance->Init();
    _instance = instance;
  }
  instance->AddConnection(conn);
  return instance;
}

void Transporter::AddConnection(std::weak_ptr<Connection> conn) {
  std::lock_guard<std::mutex> lock(_conn_mutex);
  _connections.emplace_back(conn);
}

void Transporter::AddSendRequest(SendRequest req) {
  _collector.Add(req);
}

void Transporter::CollectActiveSockets() {
  std::vector<std::shared_ptr<Connection> > connections;
  std::vector<std::shared_ptr<SocketObject> > sockets;
  {
    std::lock_guard<std::mutex> lock(_conn_mutex);
    for(auto it = _connections.begin(); it != _connections.end(); ) {
      std::shared_ptr<Connection> conn = (*it).lock();
      if(conn) {
        connections.push_back(conn);
        ++it;
      }
      else
        it = _connections.erase(it);
    }
  }

  for(auto conn : connections) {
    conn->GetActiveSockets(sockets);
  }
  RunOnce(sockets);
}

bool Transporter::RunOnce(std::vector<std::shared_ptr<SocketObject> >& objects) {
  fd_set rfds, wfds;
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);

  int res = Select(rfds, wfds, objects);

  if(res < 0 ) {
    return false;
  }

  UpdateSendRequests();
  int msgs_received = HandleReceive(objects, rfds);
  bool msgs_sent = HandleSend(wfds);
  return ( msgs_received || msgs_sent);
}

void Transporter::UpdateSendRequests() {

  std::vector<SendRequest> vec;
  _collector.Collect(vec);
  _req_vec.insert(_req_vec.end(), vec.begin(), vec.end());

  if(_req_vec.size()) {
    for(size_t i = _req_vec.size() -1; ; --i) {
      if(!_req_vec.at(i)._obj.Lock())
        _req_vec.erase(_req_vec.begin() + i);
      if(!i)
        break;
    }
  }
}

int Transporter::HandleReceive(std::vector<std::shared_ptr<SocketObject> >& objects, fd_set& rfds) {
  int processed = 0;

  for(auto obj : objects) {
    if(FD_ISSET(obj->Handle(), &rfds) || (obj->GetSession() && obj->GetSession()->HasReadPending())) {
      obj->GetConnection()->ProcessSocket(obj);
      ++processed;
    }
  }

  return processed;
}

bool Transporter::HandleSend(fd_set& wfds) {
  std::vector<SendRequest> resend_vec;
  std::set<int> resend_soc;

  if(!_req_vec.size())
    return false;

  for(auto req : _req_vec) {
    auto client = req._obj.Get();
    int socket = client->Handle();
    bool resend = false;

    if((FD_ISSET(socket, &wfds)) && (resend_soc.find(socket) == resend_soc.end())) {
      if(!client->GetConnection()->Write(client, req._msg)) {
        resend = true;
        resend_soc.insert(socket);
      }
    }
    else
      resend = true;

    if(resend) {
      req._obj.Unlock();
      resend_vec.push_back(req);
    }
  }

  _req_vec = resend_vec;
  return true;
}

int Transporter::Select(fd_set& rfds, fd_set& wfds, std::vector<std::shared_ptr<SocketObject> >& objects) {
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = TRANSPORTER_SELECT_TIMEOUT_IN_MS * 1000;

  int max_fd = -1;

  for(auto obj : objects) {
    int fd = obj->Handle();
    FD_SET(fd, &rfds);
    FD_SET(fd, &wfds);
    if(fd > max_fd)
      max_fd = fd;
  }

  if(max_fd < 0)
    return -1;
  int res = select(max_fd +1, &rfds, &wfds, NULL, &timeout);
  if(res < 0 )
    DLOG(warn, "Transporter : select failed");
  return res;
}
