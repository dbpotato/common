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

#include "WebsocketMessageBuilder.h"
#include "WebsocketMessage.h"
#include "WebsocketHeader.h"
#include "Logger.h"

#include <cstring>
#include <algorithm>
#include <sstream>
#include <string>


const int START_SIZE = 2;
const int LONGER_PAYLOAD_SIZE = 2;
const int LONGEST_PAYLOAD_SIZE = 8;
const int MASK_KEY_SIZE = 4;

WebsocketMessageBuilder::WebsocketMessageBuilder()
    : MessageBuilder() {
}

void WebsocketMessageBuilder::Check(std::vector<std::shared_ptr<Message> >& out_msgs) {
  MaybeGetHeaderData();

  while(_data_size && _data_size >= _expected_data_size ) {
    auto msg = CreateMessage();
    if(msg) {
      out_msgs.push_back(msg);
    }
    if(_data_size) {
      MaybeGetHeaderData();
    }
  }
}

void WebsocketMessageBuilder::MaybeGetHeaderData() {
  if(!_header) {
    _header = WebsocketHeader::MaybeCreateFromRawData(_data_size, _data);
    if(!_header) {
      DLOG(error, "WebsocketMessageBuilder : Failed to create websocket header");
      return;
    }
    _expected_data_size = _header->_header_length + _header->_final_payload_len;
  }
}

std::shared_ptr<Message> WebsocketMessageBuilder::CreateMessage() {
  std::shared_ptr<WebsocketMessage> msg;
  if(_header->_final_payload_len) {
    auto payload_data = std::shared_ptr<unsigned char>(new unsigned char[_header->_final_payload_len],
                                                       std::default_delete<unsigned char[]>());
    std::memcpy(payload_data.get(), _data.get() + _header->_header_length, _header->_final_payload_len);
    if(_header->_mask) {
      for(uint32_t i = 0; i < _header->_final_payload_len; ++i) {
        unsigned char t = payload_data.get()[i];
        t = t ^ _header->_mask_key[i % 4];
        payload_data.get()[i] = t;
      }
    }
    msg = std::make_shared<WebsocketMessage>(_header->_final_payload_len, payload_data);
  } else {
    msg = std::make_shared<WebsocketMessage>();
  }

  msg->_header = _header;

  if(!_header->_fin && ! _fragment_builder) {
    _fragment_builder = std::unique_ptr<WebsocketFragmentBuilder>(new WebsocketFragmentBuilder());
  }

  if(_fragment_builder) {
    if(!_fragment_builder->AddFragment(msg)) {
      _fragment_builder = nullptr;
    }
    msg = nullptr;
  }

  if(_header->_fin && _fragment_builder) {
    msg = _fragment_builder->MergeFragments();
    _fragment_builder = nullptr;
  }

  _header = nullptr;

  if(_data_size > _expected_data_size) {
    _data_size_cap = (_data_size - _expected_data_size)*2;
    std::shared_ptr<unsigned char> new_data(new unsigned char[_data_size_cap],
      std::default_delete<unsigned char[]>());
    std::memcpy(new_data.get(), (void*)(_data.get() + _expected_data_size), _data_size - _expected_data_size);
    _data_size = _data_size - _expected_data_size;
    _data = new_data;
    _expected_data_size = 0;
  }
  else {
    _data_size = 0;
    _data_size_cap = 0;
    _data = nullptr;
    _expected_data_size = 0;
  }
  return msg;
}
