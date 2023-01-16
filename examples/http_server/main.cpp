/*
Copyright (c) 2022 Adam Kaniewski

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


const static int SERVER_PORT = 8080;

const static std::string HTTP_RESPONSE_BODY = R"""(
<!DOCTYPE html>
<html>
  <head>
    <title>HttpServer Example</title>
  </head>
  <body>Hello World</body>
</html>
)""";

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
    request._response_msg = std::make_shared<HttpMessage>(405);
    return;
  }
  if(!request_header->GetRequestTarget().compare("/")) {
    //redirect to index.html
    request._response_msg = std::make_shared<HttpMessage>(301);
    request._response_msg->GetHeader()->SetField(HttpHeaderField::LOCATION, "/index.html");
    return;
  }
  if(!request_header->GetRequestTarget().compare("/index.html")) {
    //send http document
    request._response_msg = std::make_shared<HttpMessage>(200, HTTP_RESPONSE_BODY);
  } else {
    //send "page not found response"
    request._response_msg = std::make_shared<HttpMessage>(404);
  }
}

int main() {
  //creates base ( not encrypted ) TCP connection;
  auto connection = Connection::CreateBasic();
  auto server = std::make_shared<HttpServer>();

  if(!server->Init(connection,
                   std::make_shared<HttpRequestHandlerImpl>(),
                   SERVER_PORT)) {
    log()->error("Server failed to start at port : {}", SERVER_PORT);
    return 0;
  }

  log()->info("Server started at port : {}", SERVER_PORT);

  while(true)
    sleep(1);

  return 0;
}
