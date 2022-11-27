/*
Copyright (c) 2018 - 2022 Adam Kaniewski

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
#include "SocketContext.h"
#include "Transporter.h"
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
const int SOC_READ_BUFF_SIZE = 2048;

std::shared_ptr<Connection> Connection::CreateBasic() {
  std::shared_ptr<Connection> conn;
  conn.reset(new Connection());
  conn->Init();
  return conn;
}

Connection::Connection() {
  signal(SIGPIPE, SIG_IGN); //TODO needed ?
}

Connection::~Connection() {
}

void Connection::Init() {
  _transporter = Transporter::GetInstance();
  _connector = ConnectThread::GetInstance();
}

std::shared_ptr<SocketContext> Connection::CreateAcceptSocketContext(std::shared_ptr<Server> server) {
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
                                                 std::vector<std::weak_ptr<ClientManager> >& listeners,
                                                 bool is_raw) {
  return CreateServer(port,
                      listeners,
                      is_raw,
                      std::make_shared<SocketContext>(SocketContext::State::FINISHED));
}

std::shared_ptr<Server> Connection::CreateServer(int port,
                                                 std::vector<std::weak_ptr<ClientManager> >& listeners,
                                                 bool is_raw,
                                                 std::shared_ptr<SocketContext> context) {
  std::shared_ptr<Server> server;

  int socket = CreateServerSocket(port);
  if(socket < 0) {
    return server;
  }

  server.reset(new Server(socket,
                          shared_from_this(),
                          context,
                          listeners,
                          is_raw));
  server->Init();
  _transporter->AddSocket(server);
  _transporter->EnableSocket(server);
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

  int socket = accept(server->SocketFd(), &client_addr, &client_addr_len);
  if(socket < 0) {
    DLOG(warn, "Connection::Accept fail on socket : {}", server->SocketFd());
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
                          CreateAcceptSocketContext(server),
                          std::string(host_buf),
                          std::stoi(std::string(port_buf)),
                          {},//url
                          server));
  _connector->AddClient(client);
}

NetError Connection::AfterSocketAccepted(std::shared_ptr<SocketObject> obj) {
  return NetError::OK;
}

void Connection::NotifySocketActiveChanged(std::shared_ptr<SocketObject> obj) {
  if(obj->IsActive()) {
    _transporter->EnableSocket(obj);
  } else {
    _transporter->DisableSocket(obj);
  }
}

bool Connection::ProcessSocket(std::shared_ptr<SocketObject> obj) {
  if(obj->IsServerSocket()) {
    Accept(obj);
    return false;
  } else {
    return Read(std::static_pointer_cast<Client>(obj));
  }
}

bool Connection::Read(std::shared_ptr<Client> obj) {
  Data data;
  int read_len = 0;
  unsigned char buff[SOC_READ_BUFF_SIZE];

  bool res = SocketRead(obj, buff, SOC_READ_BUFF_SIZE, read_len);
  if (!res) {
    obj->OnConnectionClosed();
    Close(obj->SocketFd());
    return false;
  }

  if(read_len > 0) {
    std::shared_ptr<unsigned char> data_sptr(new unsigned char[read_len],
                                            std::default_delete<unsigned char[]>());
    std::memcpy(data_sptr.get(), (void*)(buff), read_len);
    data._size = read_len;
    data._data = data_sptr;
    obj->OnDataRead(data);
  }
  return (obj->GetContext()->HasReadPending()) || (read_len == SOC_READ_BUFF_SIZE);
}

bool Connection::SocketRead(std::shared_ptr<Client> obj, void* dest, int dest_size, int& out_dest_write) {
  out_dest_write = read(obj->SocketFd(), dest, dest_size);
  return (out_dest_write != 0);
}

bool Connection::SocketWrite(std::shared_ptr<Client> obj, void* buffer, int size, int& out_write_size) {
  out_write_size = write(obj->SocketFd(), buffer, size);
  return out_write_size > 0;
}

bool Connection::Write(std::shared_ptr<Client> obj, MessageWriteRequest& req) {
  bool failed = false;
  int write_size = 0;
  bool completed = false;

  std::shared_ptr<Message> msg = req._msg;

  uint32_t total_size = msg->_is_raw ? msg-> _size : msg-> _size + MESSAGE_HEADER_LENGTH;

  if(!msg->_is_raw) {
    if(req._write_offset < MESSAGE_HEADER_LENGTH) {
      uint8_t header[MESSAGE_HEADER_LENGTH] = {0};
      std::memcpy(header, &msg->_type, sizeof(msg->_type));
      std::memcpy(header + sizeof(msg->_type), &msg->_size, sizeof(msg->_size));

      if(SocketWrite(obj,
                     header + req._write_offset,
                     MESSAGE_HEADER_LENGTH - req._write_offset,
                     write_size)) {
        req._write_offset += write_size;
        if(req._write_offset < MESSAGE_HEADER_LENGTH) {
          return completed;
        }
      }
      else {
        failed = true;
      }
    }
  }

  if(msg->_size && !failed) {
    uint32_t offset = msg->_is_raw ? req._write_offset : req._write_offset - MESSAGE_HEADER_LENGTH;
    if(SocketWrite(obj,
                   msg->_data.get() + offset,
                   msg->_size - offset,
                   write_size)) {
      req._write_offset += write_size;
    }
    else {
      failed = true;
    }
  }

  if((req._write_offset == total_size) || failed) {
    completed = true;
  }

  if(completed) {
    obj->OnMsgWrite(msg, !failed);
  }

  return completed;
}

void Connection::Close(int socket_fd) {
  _transporter->RemoveSocket(socket_fd);
  close(socket_fd);
}

void Connection::SendMsg(std::shared_ptr<Client> obj, std::shared_ptr<Message> msg) {
  MessageWriteRequest req(msg);
  if(!Write(obj, req)) {
    _transporter->AddSendRequest(obj->SocketFd(), req);
  }
}
