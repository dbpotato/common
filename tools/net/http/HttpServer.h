/*
Copyright (c) 2021 Adam Kaniewski

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

#include "Client.h"

#include <memory>
#include <map>
#include <vector>

class ModuleManager;
class Server;
class Message;
class Connection;

class HttpServer;


class HttpRequest {
public:
  HttpRequest();
  void Reject();
  void SetResponse(const std::string& response);
  enum Type {UNKNOWN, GET, POST};
  Type _type;
  std::string _resource_name;
  std::string _post_msg;

  std::string _response_mime_type;
  std::shared_ptr<unsigned char> _response_data;
  size_t _response_data_size;
  std::weak_ptr<Client> _client;
  bool _handled;
  bool _valid;
};


class HttpRequestHandler {
public:
  virtual void GetResource(HttpRequest& request) = 0;
};

class HttpServer : public ClientManager
                 , public std::enable_shared_from_this<HttpServer> {

public:
  HttpServer();
  bool Init(std::shared_ptr<Connection> connection,
            std::shared_ptr<HttpRequestHandler> request_handler,
            int port);

  bool OnClientConnected(std::shared_ptr<Client> client, NetError err) override;
  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  bool IsRaw() override;

private:
  void ProcessRequest(std::shared_ptr<Client> client, std::shared_ptr<Message> msg);
  void ParseRequest(HttpRequest& req, const std::string& str);
  void SendResponse(HttpRequest& req);

  std::shared_ptr<Server> _server;
  std::shared_ptr<HttpRequestHandler> _request_handler;
};
