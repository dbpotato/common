/*
Copyright (c) 2018 - 2023 Adam Kaniewski

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

#include "Connection.h"
#include "Message.h"
#include "Client.h"
#include "Data.h"
#include "Server.h"
#include "Logger.h"
#include "SocketObject.h"
#include "SocketContext.h"
#include "ThreadLoop.h"
#include "Epool.h"
#include "ConnectThread.h"

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


const int SOC_LISTEN = 256;
const size_t SOC_READ_BUFF_SIZE = 1024*1024;

std::shared_ptr<Connection> Connection::CreateBasic() {
  std::shared_ptr<Connection> conn;
  conn.reset(new Connection());
  if(!conn->Init()) {
    return nullptr;
  }
  return conn;
}

Connection::Connection() {
  signal(SIGPIPE, SIG_IGN); //TODO needed ?
}

Connection::~Connection() {
}

bool Connection::Init() {
  _epool = Epool::GetInstance();
  if(!_epool) {
    return false;
  }
  _connector = ConnectThread::GetInstance();
  _thread_loop = std::make_shared<ThreadLoop>();
  _thread_loop->Init();
  return true;
}

std::shared_ptr<SocketContext> Connection::CreateAcceptSocketContext(int socket_fd, std::shared_ptr<Server> server) {
  return std::make_shared<SocketContext>(SocketContext::State::AFTER_ACCEPTING);
}

void Connection::CreateClient(int port,
                              const std::string& host,
                              std::weak_ptr<ClientManager> mgr) {
  CreateClient(port,
               host,
               mgr,
               std::make_shared<SocketContext>(SocketContext::State::GETTING_INFO));
}

void Connection::CreateClient(int port,
                              const std::string& host,
                              std::weak_ptr<ClientManager> mgr,
                              std::shared_ptr<SocketContext> context) {
  std::shared_ptr<Client> client;
  client.reset(new Client(DEFAULT_SOCKET,
                          shared_from_this(),
                          context,
                          {},
                          port,
                          host,
                          mgr));
  _connector->AddClient(client);
}

std::shared_ptr<Server> Connection::CreateServer(int port,
                                                 std::vector<std::weak_ptr<ClientManager> >& listeners) {
  return CreateServer(port,
                      listeners,
                      std::make_shared<SocketContext>(SocketContext::State::FINISHED));
}

std::shared_ptr<Server> Connection::CreateServer(int port,
                                                 std::vector<std::weak_ptr<ClientManager> >& listeners,
                                                 std::shared_ptr<SocketContext> context) {
  std::shared_ptr<Server> server;

  int socket = CreateServerSocket(port);
  if(socket < 0 ) {
    return server;
  }

  server.reset(new Server(socket,
                          shared_from_this(),
                          context,
                          listeners));
  server->Init();
  _epool->AddListener(server);
  _epool->SetListenerAwaitingRead(server, true);
  return server;
}

std::shared_ptr<Server> Connection::CreateServer(int port,
                                    std::shared_ptr<ClientManager> listener) {
  std::vector<std::weak_ptr<ClientManager> > listeners;
  listeners.push_back(listener);
  return CreateServer(port, listeners);
}

int Connection::CreateServerSocket(int port) {
  struct addrinfo hints;
  struct addrinfo* result = nullptr;
  struct addrinfo* rp = nullptr;
  int sfd = DEFAULT_SOCKET;
  int gai_res = -1;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = IPPROTO_TCP;


  gai_res = getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &result);

  if (gai_res != 0) {
    DLOG(error, "Connection::CreateSocket getaddrinfo: {}", gai_strerror(gai_res));
    freeaddrinfo(result);
    return -1;
  }

  for (rp = result; rp; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == DEFAULT_SOCKET) {
      continue;
    }

    fcntl(sfd, F_SETFL, O_NONBLOCK);

    int reuse = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == -1) {
      DLOG(warn, "Connection::CreateSocket set SO_REUSEADDR fail");
    } else if(bind(sfd, rp->ai_addr, rp->ai_addrlen) == -1) {
      DLOG(warn, "Connection::CreateSocket bind fail");
    } else if(listen(sfd, SOC_LISTEN) == -1) {
      DLOG(warn, "Connection::CreateSocket listen fail");
    } else {
      break;
    }

    close(sfd);
    sfd = DEFAULT_SOCKET;
  }

  freeaddrinfo(result);

  if(sfd < 0) {
    DLOG(error, "Connection::CreateSocket fail");
    return sfd;
  }

  return sfd;
}

NetError Connection::AfterSocketCreated(std::shared_ptr<SocketObject> obj) {
  return NetError::OK;
}

void Connection::Accept(std::shared_ptr<SocketObject> obj) {
  std::shared_ptr<Client> client;
  struct sockaddr client_addr;
  socklen_t client_addr_len = sizeof client_addr;

  std::shared_ptr<Server> server = std::static_pointer_cast<Server>(obj);

  int socket = accept(server->GetFd(), &client_addr, &client_addr_len);
  if(socket < 0) {
    DLOG(warn, "Connection::Accept fail on socket : {}", server->GetFd());
    return;
  }

  fcntl(socket, F_SETFL, O_NONBLOCK);

  char host_buf[NI_MAXHOST];
  char port_buf[NI_MAXSERV];

  int res = getnameinfo (&client_addr, client_addr_len,
                         host_buf, sizeof host_buf,
                         port_buf, sizeof port_buf,
                         NI_NUMERICHOST | NI_NUMERICSERV);

  if(res != 0) {
    DLOG(warn, "Connection::Accept getnameinfo fail");
  }

  client.reset(new Client(socket,
                          shared_from_this(),
                          CreateAcceptSocketContext(socket, server),
                          std::string(host_buf),
                          std::stoi(std::string(port_buf)),
                          {},//url
                          server));
  _epool->AddListener(client);
  _connector->AddClient(client);
}

NetError Connection::AfterSocketAccepted(std::shared_ptr<SocketObject> obj) {
  return NetError::OK;
}

void Connection::NotifySocketActiveChanged(std::shared_ptr<SocketObject> obj) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Connection::NotifySocketActiveChanged, shared_from_this(), obj));
    return;
  }
  if(obj->IsActive()) {
    if(HasObjectPendingWrite(obj)) {
      _epool->SetListenerAwaitingFlags(obj, true, true);
    } else {
      _epool->SetListenerAwaitingRead(obj, true);
    }
  } else {
    _epool->SetListenerAwaitingFlags(obj, false, false);
  }
}

void Connection::OnSocketReadReady(std::shared_ptr<SocketObject> obj) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Connection::OnSocketReadReady, shared_from_this(), obj));
    return;
  }

  bool connected = true;
  if(obj->IsServerSocket()) {
    Accept(obj);
  } else {
    auto client = std::static_pointer_cast<Client>(obj);
    connected = client->IsConnected();
    if(connected) {
      Read(client);
    } else {
      _connector->Continue(client);
    }
  }
  if(obj->IsValid() && obj->IsActive() && connected) {
    _epool->SetListenerAwaitingRead(obj, true);
  }
}

bool Connection::Read(std::shared_ptr<Client> obj) {
  size_t read_len = 0;
  auto buff = std::make_shared<Data>(SOC_READ_BUFF_SIZE);

  bool res = SocketRead(obj, buff->GetCurrentDataRaw(), SOC_READ_BUFF_SIZE, read_len);
  if (!res) {
    OnSocketClosed(obj);
    return false;
  }

  if(read_len > 0) {
    buff->SetCurrentSize(read_len);
    obj->OnDataRead(buff);
  }

  bool read_again = (read_len == SOC_READ_BUFF_SIZE);
  return read_again;
}

bool Connection::SocketRead(std::shared_ptr<Client> obj, void* dest, int dest_size, size_t& out_read_size) {
  ssize_t result = read(obj->GetFd(), dest, dest_size);
  out_read_size = (result > 0) ? (size_t)result : 0;
  if(result < 0) {
    if(errno == EAGAIN || errno == EWOULDBLOCK) {
      return true;
    }
  }
  return (result > 0);
}

void Connection::OnSocketWriteReady(std::shared_ptr<SocketObject> obj) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Connection::OnSocketWriteReady, shared_from_this(), obj));
    return;
  }

  auto client = std::static_pointer_cast<Client>(obj);
  if(!client->IsConnected()) {
    _connector->Continue(client);
    return;
  }

  int socket_fd = client->GetFd();
  auto it = _write_reqs.find(socket_fd);

  if(it == _write_reqs.end()) {
    return;
  }

  auto& req_vec = it->second;

  bool completed = true;
  for(auto vec_it = req_vec.begin(); vec_it != req_vec.end();) {
    auto& msg_write_req = *vec_it;
    if(!Write(client, msg_write_req, completed)) {
      OnSocketClosed(obj);
      break;
    } else if(completed) {
      vec_it = req_vec.erase(vec_it);
    } else {
      _epool->SetListenerAwaitingWrite(obj, true);
      break;
    }
  }
}

bool Connection::Write(std::shared_ptr<Client> obj, MessageWriteRequest& req, bool& completed) {
  bool failed = false;
  size_t write_size = 0;
  completed = false;
  std::shared_ptr<Data> msg_data;
  bool write_res = false;

  std::shared_ptr<Message> msg = req._msg;

  do {
    msg_data = msg->GetDataSubset(SOC_READ_BUFF_SIZE, req._write_offset);
    if(msg_data && msg_data->GetCurrentSize()) {
      write_res = SocketWrite(obj,
                    msg_data->GetCurrentDataRaw(),
                    msg_data->GetCurrentSize(),
                    write_size);
      req._total_write += write_size;
      req._write_offset += write_size;
      if(write_res) {
        if(write_size < msg_data->GetCurrentSize()) {
          break;
        }
      } else {
        failed = true;
        break;
      }
    } else {
      completed = true;
    }
  } while(msg_data && msg_data->GetCurrentSize());


  if(failed) {
    completed = true;
  }

  if(completed) {
    obj->OnMsgWrite(msg, !failed);
  }

  return !failed;
}

bool Connection::SocketWrite(std::shared_ptr<Client> obj, const void* buffer, int size, size_t& out_write_size) {
  ssize_t result = write(obj->GetFd(), buffer, size);
  out_write_size = (result > 0) ? (size_t)result : 0;
  if(result < 0) {
    if(errno == EAGAIN || errno == EWOULDBLOCK) {
      return true;
    }
  }
  return (result > 0);
}


void Connection::OnSocketClosed(std::shared_ptr<SocketObject> obj) {
  _write_reqs.erase(obj->GetFd());
  obj->OnConnectionClosed();
  _epool->RemoveListener(obj->GetFd());
}

void Connection::OnSocketDestroyed(int socket_fd) {
  _write_reqs.erase(socket_fd);
  _epool->RemoveListener(socket_fd);
}

void Connection::SendMsg(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&Connection::SendMsg, shared_from_this(), client, msg));
    return;
  }

  MessageWriteRequest req(msg);

  bool write_complete = false;
  int socket_fd = client->GetFd();

  if(!HasObjectPendingWrite(client)) {
    if(!Write(client, req, write_complete)){
      OnSocketClosed(client);
    }
    if(!write_complete) {
      _epool->SetListenerAwaitingWrite(client, true);
    }
  }

  if(!write_complete) {
    _write_reqs[socket_fd].push_back(req);
  }
}

bool Connection::HasObjectPendingWrite(std::shared_ptr<SocketObject> obj) {
  int socket_fd = obj->GetFd();

  auto it = _write_reqs.find(socket_fd);
  if(it != _write_reqs.end()) {
    if(it->second.size()) {
      return true;
    }
  }
  return false;
}