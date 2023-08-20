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
#include "HttpServer.h"
#include "HttpMessage.h"
#include "HttpHeader.h"
#include "Logger.h"
#include "FileUtils.h"

#ifdef ENABLE_SSL

#include "ConnectionMTls.h"
#include "MtlsCppWrapper.h"
using namespace MtlsCppWrapper;

#endif //ENABLE_SSL

const static int HTTP_SERVER_PORT = 80;
const static int HTTPS_SERVER_PORT = 443;

const static std::string DEFAULT_INDEX_HTML = R"""(
<!DOCTYPE html>
<html>
  <head>
    <title>HttpServer Example</title>
  </head>
  <body>Hello World</body>
</html>
)""";

const static std::string SERVER_CERT_FILE="server.crt";
const static std::string SERVER_KEY_FILE="server.key";

#ifdef ENABLE_SSL
const static bool USE_SSL = true;
#else
const static bool USE_SSL = false;
#endif //ENABLE_SSL

bool LoadCerts(std::string& out_key, std::string& out_cert) {
  bool result = true;
  result = result && FileUtils::ReadFile(SERVER_KEY_FILE, out_key);
  result = result && FileUtils::ReadFile(SERVER_CERT_FILE, out_cert);
  return result;
}

class HttpRequestHandlerImpl
    : public HttpRequestHandler {
public:
  /*
   * This method is called when message from connected client was received
   * Implementation should create HttpMessage response based on
   * incoming request.
   * If response should not be automatically send back by HttpServer
   * then HttpRequest::_handled value must be changed to 'true'
   */
  void Handle(HttpRequest& request) override;
};

void HttpRequestHandlerImpl::Handle(HttpRequest& request) {
  auto request_header = request._request_msg->GetHeader();
  if(request_header->GetMethod() != HttpHeaderMethod::GET) {
    //send "method not supported" response
    log()->warn("Got request with method other than GET");
    request._response_msg = std::make_shared<HttpMessage>(405);
    return;
  }

  auto target = request_header->GetRequestTarget();
  log()->info("Got Request for : {}", target);

  if(!target.compare("/")) {
    //redirect to index.html
    request._response_msg = std::make_shared<HttpMessage>(301);
    request._response_msg->GetHeader()->SetField(HttpHeaderField::LOCATION, "/index.html");
    return;
  } else {
    auto msg = HttpMessage::CreateFromFile("." + request_header->GetRequestTarget());
    if(msg) {
      request._response_msg = msg;
    } else {
      if(!target.compare("/index.html")) {
        //send http document
        request._response_msg = std::make_shared<HttpMessage>(200, DEFAULT_INDEX_HTML);
      } else {
        //send "page not found response"
        request._response_msg = std::make_shared<HttpMessage>(404);
      }
    }
  }
}

int main(int argc, char** args) {
  int listen_port = USE_SSL ? HTTPS_SERVER_PORT: HTTP_SERVER_PORT;
  if(argc == 2) {
    if(!StringUtils::ToInt(args[1], listen_port)) {
      log()->error("Can't parse server port : {}", args[1]); 
      return 1;
    }
  }


  std::shared_ptr<Connection> connection;
  std::shared_ptr<Server> server;
  std::shared_ptr<HttpServer> http_server = std::make_shared<HttpServer>();

#ifdef ENABLE_SSL
  std::string key, cert;
  if(!LoadCerts(key, cert)) {
    log()->error("Server failed to load cert files");
    return 1;
  }
  auto mtls_config = MtlsCppConfig::CreateServerConfig(key, cert);
  if(!mtls_config) {
    log()->error("Failed to create ssl config");
    return 1;
  }

  auto ssl_connection = ConnectionMTls::CreateMTls();
  connection = ssl_connection;
  server = ssl_connection->CreateServer(listen_port, http_server, mtls_config);
#else
  connection = Connection::CreateBasic();
  server = connection->CreateServer(listen_port, http_server);
#endif //ENABLE_SSL

  if(!http_server->Init(std::make_shared<HttpRequestHandlerImpl>(), server)) {
    log()->error("Server failed to start at port : {}", listen_port);
    return 0;
  }

  log()->info("Server started at port : {}", listen_port);

  while(true)
    sleep(1);

  return 0;
}
