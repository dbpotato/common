/*
Copyright (c) 2022 - 2023 Adam Kaniewski

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
#include "DataResource.h"
#include "WebsocketServer.h"
#include "HttpMessage.h"
#include "WebsocketMessage.h"
#include "HttpHeader.h"
#include "Logger.h"
#include "JsonMsg.h"
#include "UploadSession.h"
#include "ThreadLoop.h"
#include "MimeTypeFinder.h"
#include "WebApp.h"


const static int SERVER_PORT = 8088;
const static std::string UPLOAD_SUB_DIR = "uploaded_files";


class HttpRequestHandlerImpl
    : public HttpRequestHandler {
public:
  void Handle(HttpRequest& request) override;
};


class WebsocketClientListenerImpl
   : public WebsocketClientListener
   , public std::enable_shared_from_this<WebsocketClientListenerImpl> {
public:
  WebsocketClientListenerImpl();
  bool OnWsClientConnected(std::shared_ptr<Client> client, const std::string& request_arg);
  void OnWsClientMessage(std::shared_ptr<Client> client, std::shared_ptr<WebsocketMessage> message);
  void OnWsClientClosed(std::shared_ptr<Client> client);
private:
  std::shared_ptr<UploadSession> GetSession(std::shared_ptr<Client> client);
  std::map<uint32_t, std::shared_ptr<UploadSession>> _sessions;
  std::shared_ptr<ThreadLoop> _thread_loop;
};

void HttpRequestHandlerImpl::Handle(HttpRequest& request) {
  auto request_header = request._request_msg->GetHeader();
  std::string target = request_header->GetRequestTarget();
  if(request_header->GetMethod() != HttpHeaderMethod::GET) {
    //send "method not supported" response
    request._response_msg = std::make_shared<HttpMessage>(405);
    return;
  }
  if(!target.compare("/")) {
    //redirect to index.html
    request._response_msg = std::make_shared<HttpMessage>(301);
    request._response_msg->GetHeader()->SetField(HttpHeaderField::LOCATION, "/index.html");
    return;
  }
  if(!target.compare("/index.html") || !target.compare("/uploader.js")) {
    //send http document
    const std::string& document_body = !target.compare("/index.html") ? WebApp::INDEX_HTML : WebApp::UPLOADER_JS;
    request._response_msg = std::make_shared<HttpMessage>(200, document_body);
    request._response_msg->GetHeader()->SetField(HttpHeaderField::CONTENT_TYPE, MimeTypeFinder::Find(target));
  } else {
    request._response_msg = std::make_shared<HttpMessage>(404);
  }
}

WebsocketClientListenerImpl::WebsocketClientListenerImpl()
    : WebsocketClientListener() {
  _thread_loop = std::make_shared<ThreadLoop>();
  _thread_loop->Init();
}

bool WebsocketClientListenerImpl::OnWsClientConnected(std::shared_ptr<Client> client, const std::string& request_arg) {
  log()->info("WS Client connected, id : {}", client->GetId());
  return true;
}

void WebsocketClientListenerImpl::OnWsClientMessage(std::shared_ptr<Client> client, std::shared_ptr<WebsocketMessage> message) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebsocketClientListenerImpl::OnWsClientMessage, shared_from_this(), client, message));
    return;
  }

  auto session = GetSession(client);
  if(session) {
    session->OnWsClientMessage(message);
  } else {
    log()->critical("Failed to create UploadSession");
    std::exit(1);
  }
}

void WebsocketClientListenerImpl::OnWsClientClosed(std::shared_ptr<Client> client) {
  log()->info("WS Client : {} disconnected", client->GetId());
}

std::shared_ptr<UploadSession> WebsocketClientListenerImpl::GetSession(std::shared_ptr<Client> client) {
  std::shared_ptr<UploadSession> result;

  auto it = _sessions.find(client->GetId());
  if(it == _sessions.end()) {
    auto save_path = std::filesystem::absolute(std::filesystem::current_path()) / UPLOAD_SUB_DIR;
    result = UploadSession::Create(client, save_path);
    if(result) {
      _sessions.insert({client->GetId(), result});
    }
  } else {
    result = it->second;
  }
  return result;
}

int main() {
  auto connection = Connection::CreateBasic();
  auto server = std::make_shared<WebsocketServer>();

  if(!server->Init(connection,
                   std::make_shared<HttpRequestHandlerImpl>(),
                   std::make_shared<WebsocketClientListenerImpl>(),
                   SERVER_PORT)) {
    log()->error("Server failed to start at port : {}", SERVER_PORT);
    return 0;
  }

  log()->info("Server started at port : {}", SERVER_PORT);

  while(true) {
    sleep(1);
  }

  return 0;
}
