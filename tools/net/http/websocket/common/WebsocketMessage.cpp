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

#include "WebsocketMessage.h"
#include "Logger.h"

#include <cstring>
#include <cmath>
#include <vector>


WebsocketMessage::WebsocketMessage()
    : Message(0, nullptr)
    , _is_text_message(false) {
}

WebsocketMessage::WebsocketMessage(uint32_t size, const void* data)
    : Message(size, data)
    , _is_text_message(false) {
}

WebsocketMessage::WebsocketMessage(uint32_t size, std::shared_ptr<unsigned char> data, uint32_t offset, bool copy)
    : Message(size, data, offset, copy)
    , _is_text_message(false) {
}

WebsocketMessage::WebsocketMessage(const std::string& str)
    : Message(str)
    , _is_text_message(true) {
}

std::shared_ptr<Message> WebsocketMessage::ConvertToBaseMessage() {
  return CreateBaseMessage(_is_text_message ? WebsocketHeader::TEXT : WebsocketHeader::BINARY,
                           _size,
                           _data);
}

std::shared_ptr<Message> WebsocketMessage::CreateBaseMessage(WebsocketHeader::OpCode op_code, uint32_t data_size, std::shared_ptr<unsigned char> data) {
  int header_size_in_bytes = 2;
  uint8_t header_start = 128 + (int)op_code;
  uint8_t mask_with_payload_size = 0;
  int payload_field_bit_size = 7;
  if(data_size <= 125 ) {
    mask_with_payload_size = data_size;
  } else if(data_size > 125 ) {
    payload_field_bit_size = 16;
    header_size_in_bytes = 4;
    mask_with_payload_size = 126;
    int bit_lenght_for_data_size = (int)log2(data_size) + 1;
    if(bit_lenght_for_data_size > 16) {
      payload_field_bit_size = 64;
      header_size_in_bytes = 10;
      mask_with_payload_size = 127;
    }
  }

  std::vector<uint8_t> header_data;
  header_data.resize(header_size_in_bytes);
  std::memcpy(header_data.data(), &header_start, 1);
  std::memcpy(header_data.data() +1 , &mask_with_payload_size, 1);
  if(payload_field_bit_size > 7) {
    if(payload_field_bit_size == 16) {
      uint16_t size = data_size;
      size = htobe16(size);
      std::memcpy(header_data.data() + 2, &size, 2);
    }
    else {
      uint64_t size = data_size;
      size = htobe64(size);
      std::memcpy(header_data.data() + 2, &size, 8);
    }
  }

  uint32_t msg_data_size = header_size_in_bytes + data_size;
  auto msg_data = std::shared_ptr<unsigned char>(new unsigned char[msg_data_size], std::default_delete<unsigned char[]>());
  std::memcpy(msg_data.get(), header_data.data(), header_size_in_bytes);
  if(msg_data_size) {
    std::memcpy(msg_data.get() + header_data.size(), data.get(), data_size);
  }

  return std::make_shared<Message>(msg_data_size, msg_data);
}

std::shared_ptr<Message> WebsocketMessage::CreatePingMessage() {
  return CreateBaseMessage(WebsocketHeader::PING, 0, {});
}

std::shared_ptr<Message> WebsocketMessage::CreatePongMessage(uint32_t ping_data_size, std::shared_ptr<unsigned char> ping_data) {
  return CreateBaseMessage(WebsocketHeader::PONG, ping_data_size, ping_data);
}

std::shared_ptr<Message> WebsocketMessage::CreateCloseMessage() {
  return CreateBaseMessage(WebsocketHeader::CLOSE, 0, {});
}
