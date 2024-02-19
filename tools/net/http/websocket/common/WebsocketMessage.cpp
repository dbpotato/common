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

#include "WebsocketMessage.h"
#include "DataResource.h"
#include "Logger.h"

#include <cstring>
#include <cmath>
#include <endian.h>
#include <vector>


WebsocketMessage::WebsocketMessage(const std::string& str)
    : _header(std::make_shared<WebsocketHeader>(WebsocketHeader::OpCode::TEXT, (uint32_t)str.length()))
    , _resource(std::make_shared<DataResource>(std::make_shared<Data>(str))) {
  _header_bin_data = std::make_shared<Data>();
}

WebsocketMessage::WebsocketMessage(std::shared_ptr<WebsocketHeader> header, std::shared_ptr<DataResource> resource)
    : _header (header)
    , _resource(resource) {
  _header_bin_data = std::make_shared<Data>();
}

std::shared_ptr<WebsocketHeader> WebsocketMessage::GetHeader() {
  return _header;
}

std::shared_ptr<DataResource> WebsocketMessage::GetResource() {
  return _resource;
}

std::shared_ptr<Data> WebsocketMessage::GetDataSubset(size_t max_size, size_t offset) {
  if(!_header_bin_data) {
    DLOG(error, "No header data");
    return {};
  }

  if(!_header_bin_data->GetTotalSize()) {
    _header_bin_data = _header->GetBinaryForm();
  }
  return CreateSubsetFromHeaderAndResource(_header_bin_data, _resource, max_size, offset);
}

std::shared_ptr<WebsocketMessage> WebsocketMessage::CreatePingMessage() {
  auto header = std::make_shared<WebsocketHeader>(WebsocketHeader::OpCode::PING, 0);
  auto resource = std::make_shared<DataResource>();
  return std::make_shared<WebsocketMessage>(header, resource);
}

std::shared_ptr<WebsocketMessage> WebsocketMessage::CreatePongMessage(std::shared_ptr<Data> ping_data) {
  auto header = std::make_shared<WebsocketHeader>(WebsocketHeader::OpCode::PONG, ping_data->GetCurrentSize());
  auto resource = std::make_shared<DataResource>(ping_data);
  return std::make_shared<WebsocketMessage>(header, resource);
}

std::shared_ptr<WebsocketMessage> WebsocketMessage::CreateCloseMessage() {
  auto header = std::make_shared<WebsocketHeader>(WebsocketHeader::OpCode::CLOSE, 0);
  auto resource = std::make_shared<DataResource>();
  return std::make_shared<WebsocketMessage>(header, resource);
}
