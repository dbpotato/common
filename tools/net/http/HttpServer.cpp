/*
Copyright (c) 2020 Adam Kaniewski

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


#include "HttpServer.h"
#include "Logger.h"
#include "Server.h"
#include "Connection.h"
#include "Message.h"
#include "MessageBuilderHttp.h"
#include "MimeTypeFinder.h"

#include <sstream>
#include <vector>

std::vector<std::string> split(const std::string &s, char delim) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> elems;
  while (std::getline(ss, item, delim)) {
    if(item.length())
      elems.push_back(std::move(item));
  }
  return elems;
}


HttpRequest::HttpRequest()
  : _type(Type::UNKNOWN)
  , _response_data_size(0)
  , _handled(false)
  , _valid(true) {
}

void HttpRequest::Reject() {
  _handled = false;
  _valid = false;
}

void HttpRequest::SetResponse(const std::string& response) {
  _response_data_size = response.length();
  _response_data = std::shared_ptr<unsigned char>(new unsigned char[_response_data_size], std::default_delete<unsigned char[]>());
  std::memcpy(_response_data.get(), response.c_str(), _response_data_size);
}

HttpServer::HttpServer() {
}

bool HttpServer::Init(std::shared_ptr<Connection> connection,
                      std::shared_ptr<HttpRequestHandler> request_handler,
                      int port) {
  _request_handler = request_handler;
  _server = connection->CreateServer(port, shared_from_this());
  return (_server != nullptr);
}

bool HttpServer::OnClientConnected(std::shared_ptr<Client> client, NetError err) {
  auto msg_builder = std::unique_ptr<MessageBuilderHttp>(new MessageBuilderHttp());
  client->SetMsgBuilder(std::move(msg_builder));
  return true;
}

void HttpServer::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  ProcessRequest(client, msg);
}

bool HttpServer::IsRaw() {
  return true;
}

void HttpServer::ProcessRequest(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  HttpRequest req;
  req._client = client;
  ParseRequest(req, msg->ToString());
  if(req._type == HttpRequest::Type::UNKNOWN) {
    req._valid = false;
  } else {
    _request_handler->GetResource(req);
  }
  if(!req._handled) {
    SendResponse(req);
    client->Send(std::make_shared<Message>(req._response_data_size, req._response_data));
  }
}

void HttpServer::ParseRequest(HttpRequest& req, const std::string& str) {
  std::vector<std::string> header = split(str, ' ');
  if(header.size() < 2)
    return;

  req._resource_name = header.at(1);
  if(!req._resource_name.compare("/")) {
    req._resource_name = "/index.html";
  }

  if(!header.at(0).compare("GET")) {
    req._type = HttpRequest::Type::GET;
  }
  else if(!header.at(0).compare("POST")) {
    req._type = HttpRequest::Type::POST;
    std::size_t found = str.find("\r\n\r\n");
    if(found!=std::string::npos && found + 4 < str.length()) {
      req._post_msg = std::string(str.c_str() + found + 4);
    }
  }
}

void HttpServer::SendResponse(HttpRequest& req) {
  auto client = req._client.lock();
  if(!client)
    return;

  if(req._response_mime_type.empty()) {
    req._response_mime_type = MimeTypeFinder::Find(req._resource_name);
  }

  std::stringstream str_stream;
  if(!req._valid) {
    str_stream << "HTTP/1.1 404 Not Found\r\n"\
                  "Content-Length: 0\r\n\r\n";
  } else {  
    str_stream << "HTTP/1.1 200 OK\r\n"\
                  "Cache-Control: no-cache\r\n"\
                  "Content-Type: " << req._response_mime_type << "\r\n"\
                  "Content-Length: " << req._response_data_size << "\r\n\r\n";
  }

  std::string header = str_stream.str();
  size_t msg_size = header.length() + req._response_data_size;

  auto data = std::shared_ptr<unsigned char>(new unsigned char[msg_size], std::default_delete<unsigned char[]>());
  std::memcpy(data.get(), header.c_str(), header.length());
  if(req._response_data_size) {
    std::memcpy(data.get() + header.length(), req._response_data.get(), req._response_data_size);
  }
  client->Send(std::make_shared<Message>(msg_size, data));
}
