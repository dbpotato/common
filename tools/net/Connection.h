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

#pragma once

#include "PosixThread.h"
#include "Transporter.h"

#include <string>
#include <map>
#include <vector>
#include <mutex>

class Message;
class SocketObject;
class Client;
class Server;
struct addrinfo;

class Data {
public:
  Data();
  uint32_t _size;
  std::shared_ptr<unsigned char> _data;
};

class Connection : public std::enable_shared_from_this<Connection>
                 , public ThreadObject {

friend bool Transporter::RunOnce(std::vector<std::shared_ptr<SocketObject> >&);

public:
  Connection();
  virtual ~Connection();
  std::shared_ptr<Client> CreateClient(int port, const std::string& host);
  std::shared_ptr<Server> CreateServer(int port);

  void SendMsg(std::shared_ptr<SocketObject>, std::shared_ptr<Message> msg);
  void Accept(int listen_soc);

  void Init();
  void Stop();
  void OnThreadStarted(int thread_id) override;

protected :
  virtual int AfterSocketCreated(int soc, bool listen_soc);
  virtual int AfterSocketAccepted(int soc);
  virtual int SocketRead(int soc, void* dest, int dest_lenght);
  virtual int SocketWrite(int soc, void* buffer, int size);
  virtual void Close(int soc);

  int CreateSocket(int port,
                   const std::string& host,
                   std::string& out_ip,
                   bool is_server_socket = false);
  Data Read(int soc);
  bool Write(int soc, std::shared_ptr<Message>);
  int SyncConnect(int sfd, addrinfo* addr_info);
  void AddSocket(int socket, std::weak_ptr<SocketObject> client);
  bool CallTransporter();
  void ThreadCheck();

  PosixThread _run_thread;
  Transporter _transporter;
  std::map<int, std::weak_ptr<SocketObject> >  _sockets;
  std::mutex _client_mutex;
  int _red_buff_lenght;
};
