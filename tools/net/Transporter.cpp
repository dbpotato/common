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

#include "Transporter.h"
#include "Connection.h"
#include "Message.h"
#include "SocketObject.h"
#include "SocketContext.h"
#include "Client.h"
#include "Logger.h"

#include <set>


std::weak_ptr<Transporter> Transporter::_instance;

SocketInfo::SocketInfo(std::weak_ptr<SocketObject> object)
    : _object(object)
    , _pending_read(false)
    , _pending_write(false)
    , _active(false) {
}

std::shared_ptr<SocketObject> SocketInfo::lock() {
  return _object.lock();
}

Transporter::Transporter() {
}

void Transporter::Init() {
  _epool = std::make_shared<Epool>();
  if(!_epool->Init(shared_from_this())) {
    DLOG(error, "Transporter : Init epool failed");
  }
}

void Transporter::AddSocket(std::shared_ptr<SocketObject> socket_obj) {
  std::lock_guard<std::mutex> lock(_socket_mutex);
  if(!_epool->AddSocket(socket_obj->Handle())) {
    DLOG(error, "Transporter : Add socket to epool failed");
  }
  _sockets.insert(std::make_pair(socket_obj->Handle(), SocketInfo(socket_obj)));
}

void Transporter::RemoveSocket(int socket_fd) {
  std::lock_guard<std::mutex> lock(_socket_mutex);


  _sockets.erase(socket_fd);

  _epool->RemoveSocket(socket_fd);
}

void Transporter::RemoveSocket(std::shared_ptr<SocketObject> socket_obj) {
  std::lock_guard<std::mutex> lock_s(_socket_mutex);
  std::lock_guard<std::mutex> lock_w(_write_mutex);
  _write_reqs.erase(socket_obj->Handle());
  _sockets.erase(socket_obj->Handle());
  _epool->RemoveSocket(socket_obj->Handle());
}

void Transporter::EnableSocket(std::shared_ptr<SocketObject> socket_obj) {
  auto socket_it =_sockets.find(socket_obj->Handle());
  if(socket_it != _sockets.end()) {
     SocketInfo& wrapper = socket_it->second;
     if(!wrapper._active) {
       wrapper._active = true;
       if(wrapper._pending_read) {
         wrapper._pending_read = false;
         socket_obj->GetConnection()->ProcessSocket(socket_obj);
       }
       _epool->SetFlags(socket_obj->Handle(), true, false);
     }
  }
}

void Transporter::DisableSocket(std::shared_ptr<SocketObject> socket_obj) {
  auto socket_it =_sockets.find(socket_obj->Handle());
  if(socket_it != _sockets.end()) {
     SocketInfo& wrapper = socket_it->second;
     if(wrapper._active) {
       wrapper._active = false;
       _epool->SetFlags(socket_obj->Handle(), false, false);
     }
  }
}

void Transporter::OnSocketEvents(std::vector<std::pair<int, SocketEventListener::Event>>& events) {
   std::set<int> closed_sockets;
   std::vector<std::shared_ptr<SocketObject>> read_sockets;
   std::vector<std::shared_ptr<Client>> write_clients;
   std::vector<std::shared_ptr<Client>> closed_clients;

   {
     std::lock_guard<std::mutex> lock(_socket_mutex);
     for(auto& pair : events) {
       auto& event = pair.second;
       auto socket_it =_sockets.find(pair.first);
       if(socket_it == _sockets.end()){
         closed_sockets.insert(pair.first); //TODO needed ?
         continue;
       }
       auto wrapper = socket_it->second;
       auto socket_obj = wrapper.lock();
       if(!socket_obj) {
         closed_sockets.insert(pair.first);
         continue;
       }

       if(event.CanRead()) {
         if(socket_obj->IsActive())
           read_sockets.push_back(socket_obj);
         else
           wrapper._pending_read = true;
       }
       if(event.CanWrite()) {
         if(!socket_obj->IsServerSocket()) {
             write_clients.push_back(std::static_pointer_cast<Client>(socket_obj));
         }
       }
       if(event.Closed()) {
         if(!socket_obj->IsServerSocket()) {
           closed_clients.push_back(std::static_pointer_cast<Client>(socket_obj));
         }
       }
     }
   }

   for(auto obj : read_sockets) {
     obj->GetConnection()->ProcessSocket(obj);
   }
   if(write_clients.size()) {
     SendPending(write_clients);
   }
   for(auto obj : closed_clients) {
     obj->OnConnectionClosed();
   }
   for(auto socket_fd : closed_sockets) {
     RemoveSocket(socket_fd);
   }
}

void Transporter::SendPending(std::vector<std::shared_ptr<Client>>& clients) {
  std::lock_guard<std::mutex> lock(_write_mutex);

  for(auto client : clients) {
    int socket_fd = client->Handle();
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
  std::lock_guard<std::mutex> lock(_write_mutex);
  _write_reqs[socket_fd].push_back(msg);
  _epool->SetFlags(socket_fd, true, true);
}
