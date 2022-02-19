/*
Copyright (c) 2020 - 2022 Adam Kaniewski

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
#include "HttpHeader.h"
#include "HttpMessage.h"
#include "HttpServer.h"
#include "Message.h"
#include "MessageBuilderHttp.h"
#include "Logger.h"
#include "Server.h"

HttpRequest::HttpRequest()
  : _handled(false) {
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
  std::shared_ptr<HttpMessage> http_msg = std::static_pointer_cast<HttpMessage>(msg);
  ProcessRequest(client, http_msg);
}

bool HttpServer::IsRaw() {
  return true;
}

void HttpServer::ProcessRequest(std::shared_ptr<Client> client, std::shared_ptr<HttpMessage> msg) {
  HttpRequest request;
  request._request_msg = msg;
  request._client = client;

  if(msg->GetHeader()->GetMethod() != HttpHeaderMethod::UNKNOWN_TYPE) {
    _request_handler->Handle(request);
  }

  if(!request._handled) {
    SendResponse(request);
  }
}

void HttpServer::SendResponse(HttpRequest& request) {
  auto client = request._client.lock();
  if(!client)
    return;

  std::shared_ptr<Message> response;
  if(request._response_msg ) {
    response = request._response_msg->ConvertToBaseMessage();
  }

  if(!response) {
    auto error_msg = std::make_shared<HttpMessage>(500);
    response = error_msg->ConvertToBaseMessage();
  }

  client->Send(request._response_msg->ConvertToBaseMessage());
}
