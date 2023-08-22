/*
Copyright (c) 2023 Adam Kaniewski

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
#include "Client.h"
#include "HttpMessage.h"
#include "HttpHeader.h"
#include "DataResource.h"
#include "Logger.h"
#include "HttpMessageBuilder.h"
#include "FileUtils.h"
#include "StringUtils.h"

#ifdef ENABLE_SSL
#include "ConnectionMTls.h"
const static bool needs_ssl = true;
#else
const static bool needs_ssl = false;
#endif //ENABLE_SSL


class Loader : public std::enable_shared_from_this<Loader>
            , public ClientManager {
public:
  Loader();
  bool LoadUrl(const std::string& url);
  bool ParseUrl(const std::string& url, bool& out_is_ssl, std::string& out_host, int& out_port);
  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  void OnClientConnected(std::shared_ptr<Client> client) override;
  void OnClientClosed(std::shared_ptr<Client> client) override;
private:
  void CreateClient(int port, const std::string& host, bool needs_ssl);
  std::string _target;
  std::string _host;
  std::shared_ptr<Client> _client;
#ifdef ENABLE_SSL
  std::shared_ptr<ConnectionMTls> _ssl_connection;
#endif //ENABLE_SSL
  std::shared_ptr<Connection> _connection;
};

Loader::Loader() : _target("/") {
}

void Loader::CreateClient(int port, const std::string& host, bool needs_ssl) {
  if(needs_ssl) {
#ifdef ENABLE_SSL
    if(!_ssl_connection) {
      _ssl_connection = ConnectionMTls::CreateMTls();
    }
    _ssl_connection->CreateClient(port, host, shared_from_this());
#else
    return;
#endif
  } else {
    if(!_connection) {
      _connection = Connection::CreateBasic();
    }
    _connection->CreateClient(port, host, shared_from_this());
  }
}

bool Loader::LoadUrl(const std::string& url) {
  std::string host;
  bool needs_ssl = false;
  int port = 0;
  if(!ParseUrl(url, needs_ssl, host, port)) {
    log()->error("Couldn't parese url : {}", url); 
    return false;
  }
  CreateClient(port, host, needs_ssl);
  return true;
}

bool Loader::ParseUrl(const std::string& url, bool& out_is_ssl, std::string& out_host, int& out_port) {
  std::vector<std::string> split = StringUtils::Split(url, "://", 1);
  if(split.size() != 2) {
    return false;
  }

  std::string protocol = split.at(0);
  std::string host_target = split.at(1);

  if(!protocol.compare("http")) {
    out_is_ssl = false;
  } else if(!protocol.compare("https")) {
#ifdef ENABLE_SSL
    out_is_ssl = true;
#else
    log()->error("This app was compiled without SSL support. Set ENABLE_SSL true in CMakeLists.txt");
    return false;
#endif

  } else {
    return false;
  }

  split = StringUtils::Split(host_target, "/", 1);
  _host = split.at(0);

  std::vector<std::string> port_split = StringUtils::Split(_host, ":", 1);
  if(port_split.size() == 1) {
    out_port = out_is_ssl ? 443 : 80;
  } else if(port_split.size() == 2) {
    _host = port_split.at(0);
    if(!StringUtils::ToInt(port_split.at(1), out_port)) {
      return false;
    }
  } else {
    return false;
  }

  out_host = _host;
  if(split.size() == 2) {
    _target += split.at(1);
  }
  return true;
}

void Loader::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {  
  std::string file_name = "index.html";
  std::vector<std::string> split = StringUtils::Split(_target, "/");
  if(split.size()) {
    file_name = split.back();
  }
  auto http_msg = std::static_pointer_cast<HttpMessage>(msg);
  auto http_header = http_msg->GetHeader();
  auto http_resource = http_msg->GetResource();

  if(http_header->HasField(HttpHeaderField::LOCATION)) {
    std::string location;
    http_header->GetFieldValue(HttpHeaderField::LOCATION, location);
    log()->info("Relocate to : {}", location);
    if(!LoadUrl(location)) {
      exit(0);
    }
    return;
  }

  log()->info("Client Read : {}, loaded : {}, expected : {}",
    file_name,
    http_resource->GetSize(),
    http_resource->GetExpectedSize());

  if(!http_resource->IsCompleted()) {
    return;
  }

  log()->info("Saving : {} ...", file_name);
  bool res = http_resource->SaveToFile(file_name);
  log()->info("Saved : {} : {}", file_name, res);
  exit(0);
}

bool Loader::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  log()->info("Client Connecting : {}", (err == NetError::OK));
  auto msg_builder = std::unique_ptr<HttpMessageBuilder>(new HttpMessageBuilder());
  client->SetMsgBuilder(std::move(msg_builder));
  return true;
}

void Loader::OnClientConnected(std::shared_ptr<Client> client) {
  log()->info("Client Connected");
  _client = client;
  auto msg = std::make_shared<HttpMessage>(HttpHeaderMethod::GET, _target);
  auto header = msg->GetHeader();
  header->SetField(HttpHeaderField::HOST, _host);
  _client->Send(msg);
}

void Loader::OnClientClosed(std::shared_ptr<Client> client) {
  log()->info("Client Closed");
  exit(0);
}

int main(int argc, char** args) {
  std::string url;
  if(argc == 2) {
    url = std::string(args[1]);
  } else {
    return 0;
  }

  auto loader = std::make_shared<Loader>();
  if(!loader->LoadUrl(url)) {
    return 0;
  }

  while(true) {
    sleep(1);
  }
  return 0;    
}
