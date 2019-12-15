/*
Copyright (c) 2018 - 2019 Adam Kaniewski

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
#include "Server.h"
#include "Logger.h"
#include "SocketObject.h"
#include "BaseSessionInfo.h"

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
const int SOC_READ_BUFF_SIZE = 2048;
const int TRANSPORTER_IDDLE_SLEEP_IN_MS = 200;
const int TRANSPORTER_ACTIVE_SLEEP_IN_US = 100;


Connection::Connection() {
  signal(SIGPIPE, SIG_IGN); //TODO needed?s
}

Connection::~Connection() {
}


std::shared_ptr<BaseSessionInfo> Connection::CreateSessionInfo(bool from_accept) {
  return std::make_shared<BaseSessionInfo>(from_accept);
}

void Connection::CreateClient(int port,
                              const std::string& host,
                              std::weak_ptr<ClientManager> mgr) {
  ThreadCheck();
  std::shared_ptr<Client> client;
  client.reset(new Client(DEFAULT_SOCKET,
                          shared_from_this(),
                          {},
                          port,
                          host,
                          mgr));

  client->SetSession(CreateSessionInfo(false));
  AddUnfinishedClient(client);
}

std::shared_ptr<Server> Connection::CreateServer(int port,
                                                 std::vector<std::weak_ptr<ClientManager> >& listeners,
                                                 bool is_raw) {
  ThreadCheck();
  std::shared_ptr<Server> server;

  int socket = CreateServerSocket(port);
  if(socket < 0)
    return server;

  server.reset(new Server(socket, shared_from_this(), listeners, is_raw));
  AddSocket(server);
  return server;
}


std::shared_ptr<Server> Connection::CreateServer(int port,
                                    std::shared_ptr<ClientManager> listener) {
  std::vector<std::weak_ptr<ClientManager> > listeners;
  listeners.push_back(listener);
  return CreateServer(port, listeners, listener->IsRaw());
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
    if (sfd == DEFAULT_SOCKET)
      continue;

    fcntl(sfd, F_SETFL, O_NONBLOCK);

    int reuse = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == -1)
      DLOG(warn, "Connection::CreateSocket set SO_REUSEADDR fail");
    else if(bind(sfd, rp->ai_addr, rp->ai_addrlen) == -1)
      DLOG(warn, "Connection::CreateSocket bind fail");
    else if(listen(sfd, SOC_LISTEN) == -1)
      DLOG(warn, "Connection::CreateSocket listen fail");
    else
      break;

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

  int socket = accept(server->Handle(), &client_addr, &client_addr_len);
  if(socket < 0) {
    DLOG(warn, "Connection::Accept fail");
    return;
  }

  fcntl(socket, F_SETFL, O_NONBLOCK);


  char host_buf[NI_MAXHOST];
  char port_buf[NI_MAXSERV];

  int res = getnameinfo (&client_addr, client_addr_len,
                         host_buf, sizeof host_buf,
                         port_buf, sizeof port_buf,
                         NI_NUMERICHOST | NI_NUMERICSERV);

  if(res != 0)
    DLOG(warn, "Connection::Accept getnameinfo fail");

  client.reset(new Client(socket,
                          shared_from_this(),
                          std::string(host_buf),
                          std::stoi(std::string(port_buf)),
                          {},//url
                          server));
  client->SetSession(CreateSessionInfo(true));
  AddUnfinishedClient(client);
}

NetError Connection::AfterSocketAccepted(std::shared_ptr<SocketObject> obj) {
  return NetError::OK;
}

void Connection::Close(SocketObject* obj) {
  close(obj->Handle());
}

void Connection::ProcessSocket(std::shared_ptr<SocketObject> obj) {
  if(obj->IsServerSocket())
    Accept(obj);
  else
    Read(std::static_pointer_cast<Client>(obj));
}

void Connection::Read(std::shared_ptr<Client> obj) {
  Data data;
  int read_len = 0;
  unsigned char buff[SOC_READ_BUFF_SIZE];

  bool res = SocketRead(obj, buff, SOC_READ_BUFF_SIZE, read_len);

  if (!res) {
    obj->OnConnectionClosed();
    return;
  }

  if(read_len) { 
    std::shared_ptr<unsigned char> data_sptr(new unsigned char[read_len],
                                             std::default_delete<unsigned char[]>());
    std::memcpy(data_sptr.get(), (void*)(buff), read_len);
    data._size = read_len;
    data._data = data_sptr;
    obj->OnDataRead(data);
  }
}

bool Connection::SocketRead(std::shared_ptr<Client> obj, void* dest, int dest_size, int& out_dest_write) {
  out_dest_write = read(obj->Handle(), dest, dest_size);
  return out_dest_write > 0;
}

bool Connection::SocketWrite(std::shared_ptr<Client> obj, void* buffer, int size, int& out_write_size) {
  out_write_size = write(obj->Handle(), buffer, size);
  return out_write_size > 0;
}

bool Connection::Write(std::shared_ptr<Client> obj, std::shared_ptr<Message> msg) {
  bool failed = false;
  int write_size = 0;
  bool completed = false;

  uint32_t total_size = msg->_is_raw ? msg-> _size : msg-> _size + MESSAGE_HEADER_LENGTH;

  if(!msg->_is_raw) {
    if(msg->_write_offset < MESSAGE_HEADER_LENGTH) {
      uint8_t header[MESSAGE_HEADER_LENGTH] = {0};
      std::memcpy(header, &msg->_type, sizeof(msg->_type));
      std::memcpy(header + sizeof(msg->_type), &msg->_size, sizeof(msg->_size));

      if(SocketWrite(obj,
                     header + msg->_write_offset,
                     MESSAGE_HEADER_LENGTH - msg->_write_offset,
                     write_size)) {
        msg->_write_offset += write_size;
        if(msg->_write_offset < MESSAGE_HEADER_LENGTH) {
          return completed;
        }
      }
      else {
        failed = true;
      }
    }
  }

  if(msg->_size && !failed) {
    uint32_t offset = msg->_is_raw ? msg-> _write_offset : msg-> _write_offset - MESSAGE_HEADER_LENGTH;
    if(SocketWrite(obj,
                   msg->_data.get() + offset,
                   msg->_size - offset,
                   write_size)) {
      msg->_write_offset += write_size;
    }
    else {
      failed = true;
    }
  }

  if((msg->_write_offset == total_size) || failed )
    completed = true;

  if(completed)
    obj->OnMsgWrite(msg, !failed);


  return completed;
}

void Connection::Init() {
  _transporter.Init(shared_from_this());
  _run_thread.Run(shared_from_this(), 1);
}

void Connection::OnThreadStarted(int thread_id) {
  while(_run_thread.ShouldRun()) {
    ConnectClients();
    if(CallTransporter())
      std::this_thread::sleep_for(std::chrono::microseconds(TRANSPORTER_ACTIVE_SLEEP_IN_US));
    else
      std::this_thread::sleep_for(std::chrono::milliseconds(TRANSPORTER_IDDLE_SLEEP_IN_MS));
  }
}

void Connection::Stop() {
  _run_thread.Stop();
}

void Connection::SendMsg(std::shared_ptr<Client> obj, std::shared_ptr<Message> msg) {
  _transporter.AddSendRequest(SendRequest(obj, msg));
}

void Connection::AddSocket(std::shared_ptr<SocketObject> socket) {
  std::lock_guard<std::mutex> lock(_socket_mutex);
  _sockets.push_back(socket);
}


void Connection::AddUnfinishedClient(std::shared_ptr<Client> client) {
  std::lock_guard<std::mutex> lock(_uf_client_mutex);
  _unfinshed_clients.push_back(client);
}


void Connection::ConnectClients() {
  std::vector<std::pair<std::shared_ptr<Client>, NetError> > _clients_to_notify;

  {
    std::lock_guard<std::mutex> lock(_uf_client_mutex);

    for(auto it = _unfinshed_clients.begin(); it != _unfinshed_clients.end(); ){
      std::shared_ptr<Client> client = *it;
      auto session = std::static_pointer_cast<BaseSessionInfo>(client->GetSession());
      NetError err = session->Continue(client, shared_from_this());
      if(err != NetError::RETRY) {
        _clients_to_notify.emplace_back(client, err);
        if(err == NetError::OK)
          AddSocket(client);
        it = _unfinshed_clients.erase(it);
      }
      else {
        ++it;
      }
    }
  }

  for(auto& entry : _clients_to_notify) {
    entry.first->OnConnected(entry.second);
  }
}

bool Connection::CallTransporter() {
  std::vector<std::shared_ptr<SocketObject> > objects;
  {
    std::lock_guard<std::mutex> lock(_socket_mutex);

    for(auto it = _sockets.begin(); it != _sockets.end(); ){
      if(auto obj = it->lock()) {
        if(obj->IsActive())
          objects.push_back(obj);
        ++it;
      }
      else {
        it = _sockets.erase(it);
      }
    }
  }

  if(!objects.size())
    return false;

  return _transporter.RunOnce(objects);
}

void Connection::ThreadCheck() {
  if(!_run_thread.IsRunning())
    DLOG(warn, "Connection was not intialized");
}
