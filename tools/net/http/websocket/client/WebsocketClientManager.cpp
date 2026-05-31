/*
Copyright (c) 2026 Adam Kaniewski

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

#include <random>

#include "WebsocketClientManager.h"




#include "Connection.h"
#include "Data.h"
#include "DataResource.h"
#include "HttpHeader.h"
#include "HttpHeaderDecl.h"
#include "HttpMessage.h"
#include "HttpMessageBuilder.h"
#include "Logger.h"
#include "StringUtils.h"
#include "WebsocketDataCutter.h"
#include "WebsocketFragmentBuilder.h"
#include "WebsocketMessage.h"
#include "WebsocketMessageBuilder.h"

#include "base64.h"
#include "sha1.hpp"



const std::string HANDSHAKE_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const std::string WEBSOCKET_VERSION = "13";
const int CLIENT_KEY_RAW_SIZE = 16;


WebsocketClientManager::WebsocketClientManager() {
}

WebsocketClientManager::~WebsocketClientManager() {
}

void WebsocketClientManager::CreateClient(std::shared_ptr<Connection> connection,
                                          const std::string& host,
                                          int port,
                                          const std::string& request_target) {
  auto internal_mgr = std::make_shared<WebsocketClientManager::InternalWebsocketClientManager>(shared_from_this(),
                                                                                              host,
                                                                                              port,
                                                                                              request_target);
  internal_mgr->Init();
  connection->CreateClient(port, host, internal_mgr);
}

WebsocketClientManager::InternalWebsocketClientManager::InternalWebsocketClientManager(std::weak_ptr<ClientManager> target_manager,
                                  const std::string& host,
                                  int port,
                                  const std::string& request_target)
    : _target_manager(target_manager)
    , _host(host)
    , _port(port)
    , _request_target(request_target) {
}


void WebsocketClientManager::InternalWebsocketClientManager::Init() {
  Lock();
}

void WebsocketClientManager::InternalWebsocketClientManager::Lock() {
  _self = shared_from_this();
}

void WebsocketClientManager::InternalWebsocketClientManager::Release() {
  _self = nullptr;
}

void WebsocketClientManager::InternalWebsocketClientManager::OnFailed() {
  auto target_manger = _target_manager.lock();
  if(target_manger) {
    target_manger->OnClientConnecting(nullptr, NetError::FAILED);
  }
}

bool WebsocketClientManager::InternalWebsocketClientManager::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  return err == NetError::OK;
}

void WebsocketClientManager::InternalWebsocketClientManager::OnClientConnected(std::shared_ptr<Client> client) {
  _connecting_client = client;
  client->SetMsgBuilder(std::unique_ptr<HttpMessageBuilder>(new HttpMessageBuilder()));
  SendUpgradeRequest(client);
}

void WebsocketClientManager::InternalWebsocketClientManager::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  _connecting_client = nullptr;
  auto http_msg = std::dynamic_pointer_cast<HttpMessage>(msg);
  auto target_manager = _target_manager.lock();

  if(!target_manager) {
    return;
  }

  if(!http_msg) {
    DLOG(error, "WebsocketClientManager: expected HttpMessage during handshake");
    OnFailed();
    return;
  }

  if(HandleUpgradeResponse(client, http_msg)) {
    if(target_manager->OnClientConnecting(client, NetError::OK)) {
      target_manager->OnClientConnected(client);
    }
  } else {
    target_manager->OnClientConnecting(client, NetError::FAILED);
  }
}

void WebsocketClientManager::InternalWebsocketClientManager::OnClientClosed(std::shared_ptr<Client> client) {
}

std::string WebsocketClientManager::InternalWebsocketClientManager::GenerateClientKey() {
  unsigned char raw[CLIENT_KEY_RAW_SIZE];
  std::random_device rd;
  for(int i = 0; i < CLIENT_KEY_RAW_SIZE; ++i) {
    raw[i] = static_cast<unsigned char>(rd());
  }
  return base64_encode(raw, CLIENT_KEY_RAW_SIZE);
}

std::string WebsocketClientManager::InternalWebsocketClientManager::ExpectedAcceptHash(const std::string& key) {
  unsigned char sha1_digest[20];
  digestpp::sha1 hasher;
  hasher.absorb(key + HANDSHAKE_GUID);
  hasher.digest(sha1_digest, sizeof(sha1_digest));
  return base64_encode(sha1_digest, 20);
}

void WebsocketClientManager::InternalWebsocketClientManager::SendUpgradeRequest(std::shared_ptr<Client> client) {
  _client_key = GenerateClientKey();

  auto header = std::make_shared<HttpHeader>(HttpHeaderProtocol::HTTP_1_1, HttpHeaderMethod::GET, _request_target);
  header->SetField(HttpHeaderField::HOST, _host + ":" + std::to_string(_port));
  header->SetField(HttpHeaderField::UPGRADE, "websocket");
  header->SetField(HttpHeaderField::CONNECTION, "Upgrade");
  header->SetField(HttpHeaderField::SEC_WEBSOCKET_KEY, _client_key);
  header->SetField(HttpHeaderField::SEC_WEBSOCKET_VERSION, WEBSOCKET_VERSION);

  auto http_msg = std::make_shared<HttpMessage>(header, nullptr);
  client->Send(http_msg);
}

bool WebsocketClientManager::InternalWebsocketClientManager::HandleUpgradeResponse(std::shared_ptr<Client> client,
                                                                            std::shared_ptr<HttpMessage> msg) {
  auto header = msg->GetHeader();
  if(!header) {
    DLOG(error, "WebsocketClientManager: handshake response has no header");
    return false;
  }

  if(header->GetStatusCode() != 101) {
    DLOG(error, "WebsocketClientManager: handshake failed, status code : {}", header->GetStatusCode());
    return false;
  }

  std::string upgrade_value;
  if(!header->GetFieldValue(HttpHeaderField::UPGRADE, upgrade_value)
     || StringUtils::Lowercase(upgrade_value).compare("websocket")) {
    DLOG(error, "WebsocketClientManager: missing or invalid Upgrade header");
    return false;
  }

  std::string accept_hash;
  if(!header->GetFieldValue(HttpHeaderField::SEC_WEBSOCKET_ACCEPT, accept_hash)) {
    DLOG(error, "WebsocketClientManager: missing Sec-WebSocket-Accept");
    return false;
  }

  if(accept_hash != ExpectedAcceptHash(_client_key)) {
    DLOG(error, "WebsocketClientManager: Sec-WebSocket-Accept mismatch");
    return false;
  }

  client->SetMsgBuilder(std::unique_ptr<WebsocketMessageBuilder>(new WebsocketMessageBuilder()));
  client->SetManager(_target_manager);
  return true;
}

bool WebsocketClientManager::MaybeHandleWebsocketMessage(std::shared_ptr<Client> client,
                                                         std::shared_ptr<WebsocketMessage> msg) {
  bool handled = false;
  auto header = msg->GetHeader();
  if (!header) {
    return false;
  }

  switch(header->_opcode) {
    case WebsocketHeader::TEXT:
    case WebsocketHeader::BINARY:
      break;
    case WebsocketHeader::PING: {
      auto data = msg->GetResource() ? msg->GetResource()->GetLastRecivedData() : nullptr;
      client->Send(WebsocketMessage::CreatePongMessage(data));
      handled = true;
      break;
    }
    case WebsocketHeader::PONG:
      handled = true;
      break;
    case WebsocketHeader::CLOSE:
      client->Send(WebsocketMessage::CreateCloseMessage());
      handled = true;
      break;
    default:
      DLOG(error, "WebsocketClient: invalid op code : {}", (int)header->_opcode);
      break;
  }
  return handled;
}
