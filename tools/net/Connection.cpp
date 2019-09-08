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
const int TRANSPORTER_IDDLE_SLEEP_IN_MS = 25;
const int TRANSPORTER_ACTIVE_SLEEP_IN_US = 100;

const int CONNECT_RETRY_DELAY_IN_MS = 1;
const double CONNECT_TIMEOUT_IN_MS = 300.0;


Connection::Connection()
    : _red_buff_lenght(SOC_READ_BUFF_SIZE) {
  signal(SIGPIPE, SIG_IGN);
}

Connection::~Connection() {
}

std::shared_ptr<Client> Connection::CreateClient(int port, const std::string& host) {
  ThreadCheck();
  std::shared_ptr<Client> client;
  std::shared_ptr<SessionInfo> session;
  std::string ip;

  int socket = CreateSocket(port, host, ip);
  if(socket < 0)
    return client;

  if(!AfterSocketCreated(socket, session)) {
    close(socket);
    return client;
  }

  client.reset(new Client(socket, shared_from_this(), ip, port, host));
  client->SetSession(session);

  AddSocket(socket, client);
  return client;
}

std::shared_ptr<Server> Connection::CreateServer(int port) {
  ThreadCheck();
  std::shared_ptr<Server> server;
  std::string ip;
  int socket = CreateSocket(port, {}, ip, true);
  if(socket < 0)
    return server;

  server.reset(new Server(socket, shared_from_this()));
  AddSocket(socket, server);
  return server;
}


int Connection::CreateSocket(int port, const std::string& host, std::string& out_ip, bool is_server_socket) {
  struct addrinfo hints;
  struct addrinfo* result = nullptr;
  struct addrinfo* rp = nullptr;
  int sfd = -1;
  int gai_res = -1;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = IPPROTO_TCP;

  if(is_server_socket)
    gai_res = getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &result);
  else
    gai_res = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
  if (gai_res != 0) {
    DLOG(error, "Connection::CreateSocket getaddrinfo: {}", gai_strerror(gai_res));
    freeaddrinfo(result);
    return -1;
  }

  for (rp = result; rp; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1)
      continue;

    fcntl(sfd, F_SETFL, O_NONBLOCK);

    if(!is_server_socket) {
      int connect_res = SyncConnect(sfd,rp);
      if(!connect_res)
        break;
      else if(connect_res == -1) {
        DLOG(warn, "Connection::CreateSocket connect fail : {}:{} : {}",
             host,
             port,
             strerror(errno));
      }
    }
    else {
      int reuse = 1;
      if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == -1)
        DLOG(warn, "Connection::CreateSocket set SO_REUSEADDR fail");
      else if(bind(sfd, rp->ai_addr, rp->ai_addrlen) == -1)
        DLOG(warn, "Connection::CreateSocket bind fail");
      else if(listen(sfd, SOC_LISTEN) == -1)
        DLOG(warn, "Connection::CreateSocket listen fail");
      else
        break;
    }

    close(sfd);
    sfd = -1;
  }

  if(rp) {
    char ipstr[INET_ADDRSTRLEN];
    struct sockaddr_in* ipv = reinterpret_cast<struct sockaddr_in*>(rp->ai_addr);
    struct in_addr* addr = &(ipv->sin_addr);
    inet_ntop(rp->ai_family, addr, ipstr, sizeof ipstr);
    out_ip = std::string(ipstr);
  }

  freeaddrinfo(result);

  if(sfd < 0) {
    DLOG(error, "Connection::CreateSocket fail");
    return sfd;
  }

  return sfd;
}

bool Connection::AfterSocketCreated(int socket,
                                    std::shared_ptr<SessionInfo>& session) {
  return true;
}

void Connection::Accept(std::shared_ptr<SocketObject> obj) {
  std::shared_ptr<Client> client;
  std::shared_ptr<SessionInfo> session;
  struct sockaddr client_addr;
  socklen_t client_addr_len = sizeof client_addr;

  std::shared_ptr<Server> server = std::static_pointer_cast<Server>(obj);

  int socket = accept((int)server->Handle(), &client_addr, &client_addr_len);
  if(socket < 0) {
    DLOG(warn, "Connection::Accept fail");
    return;
  }

  if(!AfterSocketAccepted(socket, session)) {
    DLOG(warn, "Connection::AfterSocketAccepted fail");
    close(socket);
    return;
  }

  char host_buf[NI_MAXHOST];
  char port_buf[NI_MAXSERV];

  int res = getnameinfo (&client_addr, client_addr_len,
                         host_buf, sizeof host_buf,
                         port_buf, sizeof port_buf,
                         NI_NUMERICHOST | NI_NUMERICSERV);
  if(res != 0)
    DLOG(error, "Connection::Accept getnameinfo fail");


  client.reset(new Client(socket,
                          shared_from_this(),
                          std::string(host_buf),
                          std::stoi(std::string(port_buf))));
  client->SetSession(session);

  AddSocket(socket, client);
  server->OnClientConnected(client);
}

bool Connection::AfterSocketAccepted(int socket, std::shared_ptr<SessionInfo>& session) {
  return true;
}

void Connection::Close(SocketObject* obj) {
  close((int)obj->Handle());
}

void Connection::Read(std::shared_ptr<SocketObject> obj) {
  Data data;
  int read_len = 0;
  unsigned char buff[_red_buff_lenght];

  bool res = SocketRead(obj, buff, _red_buff_lenght, read_len);

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

bool Connection::SocketRead(std::shared_ptr<SocketObject> obj, void* dest, int dest_size, int& out_dest_write) {
  out_dest_write = read((int)obj->Handle(), dest, dest_size);
  return out_dest_write > 0;
}

bool Connection::SocketWrite(std::shared_ptr<SocketObject> obj, void* buffer, int size, int& out_write_size) {
  out_write_size = write((int)obj->Handle(), buffer, size);
  return out_write_size > 0;
}

bool Connection::Write(std::shared_ptr<SocketObject> obj, std::shared_ptr<Message> msg) {
  bool failed = false;
  int write_size = 0;
  bool completed = false;

  uint32_t total_size = msg->_is_raw ? msg-> _size : msg-> _size + MESSAGE_HEADER_LENGTH;

  if(!msg->_is_raw) {
    if(msg->_write_offset < MESSAGE_HEADER_LENGTH) {
      uint8_t header[MESSAGE_HEADER_LENGTH];
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
      DLOG(error, "Connection::Write {}", write_size);
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
    if(CallTransporter())
      std::this_thread::sleep_for(std::chrono::microseconds(TRANSPORTER_ACTIVE_SLEEP_IN_US));
    else
      std::this_thread::sleep_for(std::chrono::milliseconds(TRANSPORTER_IDDLE_SLEEP_IN_MS));
  }
}

void Connection::Stop() {
  _run_thread.Stop();
}

void Connection::SendMsg(std::shared_ptr<SocketObject> obj, std::shared_ptr<Message> msg) {
  _transporter.AddSendRequest(SendRequest(obj, msg));
}

int Connection::SyncConnect(int sfd, addrinfo* addr_info) {
  int res = 0;
  double time_passed = 0.0;

  auto start = std::chrono::system_clock::now();

  do {
    if(res)
      std::this_thread::sleep_for(std::chrono::milliseconds(CONNECT_RETRY_DELAY_IN_MS));
    res = connect(sfd, addr_info->ai_addr, addr_info->ai_addrlen);
    time_passed = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now() - start).count();
  }
  while((res != 0) &&
        ((errno == EINPROGRESS) || (errno ==EALREADY)) &&
        (time_passed < CONNECT_TIMEOUT_IN_MS));

  if((res != 0) && (time_passed > CONNECT_TIMEOUT_IN_MS)) {
    DLOG(warn, "Connection : connect timed out");
    return -2;
  }

  return res;
}

void Connection::AddSocket(int socket, std::weak_ptr<SocketObject> client) {
  std::lock_guard<std::mutex> lock(_client_mutex);
  _sockets.insert(std::make_pair(socket, client));
}


bool Connection::CallTransporter() {

  std::vector<std::shared_ptr<SocketObject> > objects;
  {
    std::lock_guard<std::mutex> lock(_client_mutex);

    auto it = _sockets.begin();
    auto it_end = _sockets.end();

    while(it != it_end) {
      if(auto obj = it->second.lock()) {
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
