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

#include "HttpHeaderDecl.h"
#include "HttpHeader.h"
#include "HttpMessage.h"
#include "Logger.h"
#include "Message.h"
#include "Server.h"
#include "StringUtils.h"
#include "WebsocketMessageBuilder.h"
#include "WebsocketMessage.h"
#include "WebsocketServer.h"

#include "base64.h"
#include "sha1.hpp"


const std::string HANDSHAKE_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

WebsocketClientManager::WebsocketClientManager(std::shared_ptr<WebsocketServer> owner)
    : _owner(owner) {
}

bool WebsocketClientManager::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  //Should never be called
  return false;
}
void WebsocketClientManager::OnClientConnected(std::shared_ptr<Client> client) {
  //Should never be called
}

void WebsocketClientManager::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  auto server = _owner.lock();
  if(!server) {
    return;
  }

  std::shared_ptr<WebsocketMessage> websocket_msg = std::static_pointer_cast<WebsocketMessage>(msg);
  auto header = websocket_msg->_header;

  switch (header->_opcode) {
    case WebsocketHeader::TEXT:
    case WebsocketHeader::BINARY:
      server->OnWsMessage(client, websocket_msg);
      break;
    case WebsocketHeader::CLOSE:
      server->OnWsClose(client);
      break;
    case WebsocketHeader::PING:
      server->OnWsPing(client, websocket_msg);
      break;
    case WebsocketHeader::PONG:
      server->OnWsPong(client);
      break;
    default:
      DLOG(error, "Invalid op code : {}", header->_opcode);
      break;
  }
}

void WebsocketClientManager::OnClientClosed(std::shared_ptr<Client> client) {

}

bool WebsocketClientManager::IsRaw() {
  return true;
}

WebsocketServer::WebsocketServer()
    : HttpServer() {
}

bool WebsocketServer::Init(std::shared_ptr<Connection> connection,
            std::shared_ptr<HttpRequestHandler> request_handler,
            std::shared_ptr<WebsocketClientListener> ws_client_listener,
            int port) {
  auto this_sptr = std::static_pointer_cast<WebsocketServer>(shared_from_this());
  _ws_client_manager = std::make_shared<WebsocketClientManager>(this_sptr);
  _ws_client_listener = ws_client_listener;
  return HttpServer::Init(connection, request_handler, port);
}

void WebsocketServer::ProcessRequest(std::shared_ptr<Client> client, std::shared_ptr<HttpMessage> msg) {

  if(CheckForProtocolUpgradeRequest(client, msg->GetHeader())) {
    return;
  }
  HttpServer::ProcessRequest(client, msg);
}

bool WebsocketServer::CheckForProtocolUpgradeRequest(std::shared_ptr<Client> client, std::shared_ptr<HttpHeader> http_header) {
  std::string upgrade_str;
  std::string websocket_key;

  if(http_header->GetFieldValue(HttpHeaderField::UPGRADE, upgrade_str)){
    if(StringUtils::Lowercase(upgrade_str).compare("websocket")) {
      return false;
    }
  } else {
    return false;
  }

  if(!_ws_client_listener->OnWsClientConnected(client, http_header->GetRequestTarget())) {
    _server->RemoveClient(client);
    return true;
  }

  auto msg_builder = std::unique_ptr<WebsocketMessageBuilder>(new WebsocketMessageBuilder());
  client->SetMsgBuilder(std::move(msg_builder));
  client->SetManager(_ws_client_manager);

  if(http_header->GetFieldValue(HttpHeaderField::SEC_WEBSOCKET_KEY, websocket_key)) {
    std::string accept_hash = PrepareWebSocketAccept(websocket_key);
    SendHandshakeResponse(client, accept_hash);
  } else {
    DLOG(warn, "WebsocketServer::OnUpgradeRequest : cant find SEC_WEBSOCKET_KEY :\n{}", http_header->ToString());
    return false;
  }
  return true;
}

std::string WebsocketServer::PrepareWebSocketAccept(const std::string& key) {
  unsigned char sha1_digest[20];
  digestpp::sha1 hasher;
  hasher.absorb(key + HANDSHAKE_GUID);
  hasher.digest(sha1_digest, sizeof(sha1_digest));
  return base64_encode(sha1_digest,20);
}

void WebsocketServer::SendHandshakeResponse(std::shared_ptr<Client> client, const std::string& accept_hash) {
  HttpHeader header(HttpHeaderProtocol::HTTP_1_1, 101);
  header.AddField(HttpHeaderField::UPGRADE, "websocket");
  header.AddField(HttpHeaderField::CONNECTION, "Upgrade");
  header.AddField(HttpHeaderField::SEC_WEBSOCKET_ACCEPT, accept_hash);

  auto header_str = header.ToString();

  client->Send(std::make_shared<Message>(header_str));
}


void WebsocketServer::OnWsClose(std::shared_ptr<Client> client) {
  _ws_client_listener->OnWsClientClosed(client);
}

void WebsocketServer::OnWsPing(std::shared_ptr<Client> client, std::shared_ptr<WebsocketMessage> msg) {
  auto pong_msg = WebsocketMessage::CreatePongMessage(msg->_size, msg->_data);
  client->Send(pong_msg);
}

void WebsocketServer::OnWsPong(std::shared_ptr<Client> client) {
}

void WebsocketServer::OnWsMessage(std::shared_ptr<Client> client, std::shared_ptr<WebsocketMessage> msg) {
  _ws_client_listener->OnWsClientMessage(client, msg);
}
