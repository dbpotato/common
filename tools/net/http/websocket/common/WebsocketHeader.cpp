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

#include "WebsocketHeader.h"
#include "Logger.h"

#include <bitset>
#include <endian.h>


const int START_SIZE = 2;
const int LONGER_PAYLOAD_SIZE = 2;
const int LONGEST_PAYLOAD_SIZE = 8;
const int MASK_KEY_SIZE = 4;

template <size_t size>
std::bitset<size> extract_bits(std::bitset<size> x, int offset, int count) {
    x <<= offset;
    x >>= (size-count);
    return x;
}

WebsocketHeader::WebsocketHeader()
    : _fin(0)
    , _rsv1(0)
    , _rsv2(0)
    , _rsv3(0)
    , _opcode(0)
    , _mask(0)
    , _payload_len(0)
    , _final_payload_len(0)
    , _header_length(0) {
  _mask_key[0] = _mask_key[1] = _mask_key[2] = _mask_key[3] = 0;
}

WebsocketHeader::WebsocketHeader(OpCode code, uint32_t data_length)
    : WebsocketHeader() {
  _opcode = (uint8_t)code;
  _final_payload_len = data_length;
  CreateBinaryForm();
}

bool WebsocketHeader::HasControlOpCode() {
  return (_opcode >= (uint8_t) OpCode::CLOSE) && (_opcode <= (uint8_t) OpCode::PONG);
}

bool WebsocketHeader::HasFinFlag() {
  return _fin;
}

std::shared_ptr<Data> WebsocketHeader::GetBinaryForm() {
  return _binary_form;
}

void WebsocketHeader::CreateBinaryForm() {
  int header_size_in_bytes = 2;
  uint8_t header_start = 128 + _opcode;
  uint8_t mask_with_payload_size = 0;
  int payload_field_bit_size = 7;
  if(_final_payload_len <= 125 ) {
    mask_with_payload_size = _final_payload_len;
  } else if(_final_payload_len > 125 ) {
    payload_field_bit_size = 16;
    header_size_in_bytes = 4;
    mask_with_payload_size = 126;
    int bit_lenght_for_data_size = (int)log2(_final_payload_len) + 1;
    if(bit_lenght_for_data_size > 16) {
      payload_field_bit_size = 64;
      header_size_in_bytes = 10;
      mask_with_payload_size = 127;
    }
  }

  auto header_data = std::shared_ptr<unsigned char>(new unsigned char[header_size_in_bytes],
                                          std::default_delete<unsigned char[]>());
  std::memcpy(header_data.get(), &header_start, 1);
  std::memcpy(header_data.get() +1 , &mask_with_payload_size, 1);
  if(payload_field_bit_size > 7) {
    if(payload_field_bit_size == 16) {
      uint16_t size = _final_payload_len;
      size = htobe16(size);
      std::memcpy(header_data.get() + 2, &size, 2);
    }
    else {
      uint64_t size = _final_payload_len;
      size = htobe64(size);
      std::memcpy(header_data.get() + 2, &size, 8);
    }
  }
  _binary_form = std::make_shared<Data>(header_size_in_bytes, header_data);
}

std::shared_ptr<WebsocketHeader> WebsocketHeader::MaybeCreateFromRawData(std::shared_ptr<Data> data) {
  std::shared_ptr<WebsocketHeader> header = std::make_shared<WebsocketHeader>();
  if(data->GetCurrentSize() < START_SIZE) {
    return {};
  }

  auto data_buff =  data->GetCurrentDataRaw();

  header->ParseFirstBytes(data_buff[0], data_buff[1]);
  if(!header->FindPayloadAndHeaderSize(data)) {
    return {};
  }
  header->FindMaskKey(data);
  data->AddOffset(header->_header_length);

  return header;
}

void WebsocketHeader::ParseFirstBytes(uint8_t header_start, uint8_t mask_with_payload) {
  std::bitset<8> start_set(header_start);
  std::bitset<8> mask_with_payload_set(mask_with_payload);
  _fin = start_set[7];
  _rsv1 = start_set[6];
  _rsv2 = start_set[5];
  _rsv3 = start_set[4];
  _opcode = extract_bits(start_set, 4, 4).to_ulong();
  _mask = mask_with_payload_set[7];
  _payload_len = extract_bits(mask_with_payload_set, 1, 7).to_ulong();
}

bool WebsocketHeader::FindPayloadAndHeaderSize(std::shared_ptr<Data> data) {
  int payload_extra_size = 0;

  if(_payload_len == 126) {
    if(data->GetCurrentSize() < START_SIZE + LONGER_PAYLOAD_SIZE) {
      DLOG(error, "Recived data amount is less than excepted : {} vs {}", data->GetCurrentSize(), START_SIZE + LONGER_PAYLOAD_SIZE);
      return false;
    }
    uint16_t longer_payload = 0;
    std::memcpy(&longer_payload, data->GetCurrentDataRaw() + START_SIZE, LONGER_PAYLOAD_SIZE);
    longer_payload = be16toh(longer_payload);
    _final_payload_len = (uint32_t)longer_payload;
    payload_extra_size = LONGER_PAYLOAD_SIZE;
  } else if(_payload_len == 127) {
    if(data->GetCurrentSize() < START_SIZE + LONGEST_PAYLOAD_SIZE) {
      DLOG(error, "Recived data amount is less than excepted : {} vs {}", data->GetCurrentSize(), START_SIZE + LONGEST_PAYLOAD_SIZE);
      return false;
    }
    uint64_t longest_payload = 0;
    std::memcpy(&longest_payload, data->GetCurrentDataRaw() + START_SIZE, LONGEST_PAYLOAD_SIZE);
    longest_payload = be64toh(longest_payload);
    _final_payload_len = (uint32_t)longest_payload;
    payload_extra_size = LONGEST_PAYLOAD_SIZE;
  } else {
    _final_payload_len = (uint32_t)_payload_len;
  }

  _header_length = START_SIZE + payload_extra_size + (_mask ? MASK_KEY_SIZE : 0);

  return (_header_length <= data->GetCurrentSize());
}

void WebsocketHeader::FindMaskKey(std::shared_ptr<Data> data) {
  if(_mask) {
    std::memcpy(_mask_key, data->GetCurrentDataRaw() + _header_length - MASK_KEY_SIZE , MASK_KEY_SIZE);
  }
}
