/*
Copyright (c) 2019 - 2021 Adam Kaniewski

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
#include "SocketContext.h"
#include "ThreadLoop.h"
#include "Client.h"
#include "Logger.h"

#include <set>


std::weak_ptr<Transporter> Transporter::_instance;

SocketInfo::SocketInfo(std::weak_ptr<SocketObject> object)
    : _object(object)
    , _pending_read(false)
    , _pending_write(false)
    , _active(false)
    , _added_to_epool(false) {
}

std::shared_ptr<SocketObject> SocketInfo::lock() {
  return _object.lock();
}

Transporter::Transporter() {
}

void Transporter::Init() {
  _thread_loop = std::make_shared<ThreadLoop>();
  _thread_loop->Init();

  _epool = std::make_shared<Epool>();
  if(!_epool->Init(shared_from_this())) {
    DLOG(error, "Transporter : Init epool failed");
  }
}

void Transporter::AddSocket(std::shared_ptr<SocketObject> socket_obj) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Transporter::AddSocket, shared_from_this(), socket_obj));
    return;
  }

  int socket_fd = socket_obj->SocketFd();
  auto socket_it =_sockets.find(socket_fd);
  if(socket_it !=_sockets.end()) {
    DLOG(warn, "Transporter : Replacing socket object : {}", socket_fd);
    SocketInfo& wrapper = socket_it->second;
    if(wrapper._added_to_epool) {
      _epool->RemoveSocket(socket_fd);
    }
    socket_it->second = SocketInfo(socket_obj);
  }
  else {
    _sockets.insert(std::make_pair(socket_fd, SocketInfo(socket_obj)));
  }
}

void Transporter::RemoveSocket(int socket_fd) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Transporter::RemoveSocket, shared_from_this(), socket_fd));
    return;
  }

  _write_reqs.erase(socket_fd);

  auto socket_it =_sockets.find(socket_fd);
  if(socket_it == _sockets.end()) {
    DLOG(error,"Transporter : InternalRemoveSocket - fd not found : {}", socket_fd);
    return;
  }

  SocketInfo& wrapper = socket_it->second;
  if(wrapper._added_to_epool) {
    _epool->RemoveSocket(socket_fd);
  }

  _sockets.erase(socket_it);
}

void Transporter::EnableSocket(std::shared_ptr<SocketObject> socket_obj) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Transporter::EnableSocket, shared_from_this(), socket_obj));
    return;
  }

  auto socket_it =_sockets.find(socket_obj->SocketFd());
  if(socket_it == _sockets.end()) {
    DLOG(error,"Transporter : EnableSocket - fd not found : {}", socket_obj->SocketFd());
    return;
  }

  SocketInfo& wrapper = socket_it->second;
  if(!wrapper._added_to_epool) {
    if(!_epool->AddSocket(socket_obj->SocketFd())) {
      DLOG(error, "Transporter : Add socket to epool failed : {}", socket_obj->SocketFd());
      return;
    }
    wrapper._added_to_epool = true;
  }

  if(!wrapper._active) {
    wrapper._active = true;
    if(wrapper._pending_read) {
      wrapper._pending_read = false;
      socket_obj->GetConnection()->ProcessSocket(socket_obj);
    }
    _epool->SetFlags(socket_obj->SocketFd(), true, false);
  }
}

void Transporter::DisableSocket(std::shared_ptr<SocketObject> socket_obj) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Transporter::DisableSocket, shared_from_this(), socket_obj));
    return;
  }

  auto socket_it =_sockets.find(socket_obj->SocketFd());
  if(socket_it == _sockets.end()) {
    DLOG(error,"Transporter : DisableSocket - fd not found : {}", socket_obj->SocketFd());
    return;
  }

  SocketInfo& wrapper = socket_it->second;
  if(wrapper._active) {
    wrapper._active = false;
    if(wrapper._added_to_epool) {
      _epool->SetFlags(socket_obj->SocketFd(), false, false);
    }
  }
}

void Transporter::OnSocketEvents(std::vector<std::pair<int, SocketEventListener::Event>>& events) {
   if(_thread_loop->OnDifferentThread()) {
     _thread_loop->Post(std::bind(&Transporter::OnSocketEvents, shared_from_this(), events));
     return;
   }

   for(auto& pair : events) {
     int socket_fd = pair.first;
     SocketEventListener::Event& event = pair.second;

     auto socket_it =_sockets.find(socket_fd);
     if(socket_it == _sockets.end()) { //TODO needed ?
       DLOG(warn, "Transporter : OnSocketEvents - SocketObject not found : {}", socket_fd);
       RemoveSocket(socket_fd);
       continue;
     }

     auto wrapper = socket_it->second;
     auto socket_obj = wrapper.lock();
     if(!socket_obj) { //TODO needed ?
       DLOG(warn, "Transporter : OnSocketEvents - SocketObject already released : {}", socket_fd);
       RemoveSocket(socket_fd);
       continue;
     }

     if(event.CanRead()) {
       if(socket_obj->IsActive()) {
         socket_obj->GetConnection()->ProcessSocket(socket_obj);
       }
       else {
         wrapper._pending_read = true;
       }
     }

     if(event.CanWrite()) {
       if(!socket_obj->IsServerSocket()) {
         SendPending(std::static_pointer_cast<Client>(socket_obj));
       }
     }

     if(event.Closed()) {
       if(!socket_obj->IsServerSocket()) {
         (std::static_pointer_cast<Client>(socket_obj))->OnConnectionClosed();
       }
     }
   }

   _epool->NotifyEventsHandled();
}

void Transporter::SendPending(std::shared_ptr<Client> client) {
  int socket_fd = client->SocketFd();
  auto it = _write_reqs.find(socket_fd);
  auto& req_vec = it->second;

  for(auto vec_it = req_vec.begin(); vec_it != req_vec.end();) {
    auto& msg = *vec_it;
    if(!client->GetConnection()->Write(client, msg)) {
      break;
    }
    else {
      vec_it = req_vec.erase(vec_it);
    }
  }

  if(!req_vec.size()) {
    _write_reqs.erase(socket_fd);
    _epool->SetFlags(socket_fd, true, false);
  }
}

std::shared_ptr<Transporter> Transporter::GetInstance() {
  std::shared_ptr<Transporter> instance;
  if(!(instance =_instance.lock())) {
    instance.reset(new Transporter());
    instance->Init();
    _instance = instance;
  }
  return instance;
}

void Transporter::AddSendRequest(int socket_fd, std::shared_ptr<Message> msg) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Transporter::AddSendRequest, shared_from_this(), socket_fd, msg));
    return;
  }
  _write_reqs[socket_fd].push_back(msg);
  _epool->SetFlags(socket_fd, true, true);
}
