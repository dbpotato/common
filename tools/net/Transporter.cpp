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
#include "Logger.h"


const int SELECT_TIMEOUT_IN_MS = 250;


Transporter::Transporter() {
}

void Transporter::Init(std::shared_ptr<Connection> connection) {
  _connection = connection;
}

void Transporter::AddSendRequest(SendRequest req) {
  _collector.Add(req);
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

  for(auto obj : objects) {
    if(FD_ISSET((int)obj->Handle(), &rfds)) {
      if(!obj->IsServerSocket()) {
        _connection->Read(obj);
      }
      else {
        _connection->Accept(obj);
      }
    }
  }

  std::vector<SendRequest> resend_vec;

  for(auto req : _req_vec) {
    std::shared_ptr<SocketObject> obj = req._obj.Get();
    int socket = (int)obj->Handle();
    bool resend = false;

    if(FD_ISSET(socket, &wfds)) {
      if(!_connection->Write(obj, req._msg))
        resend = true;
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

int Transporter::Select(fd_set& rfds, fd_set& wfds, std::vector<std::shared_ptr<SocketObject> >& objects) {
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = SELECT_TIMEOUT_IN_MS * 1000;

  int max_fd = -1;

  for(auto obj : objects) {
    int fd = (int)obj->Handle();
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
