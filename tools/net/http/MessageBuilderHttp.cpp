/*
Copyright (c) 2021 - 2022 Adam Kaniewski

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


#include "MessageBuilderHttp.h"
#include "HttpMessage.h"
#include "Logger.h"
#include "StringUtils.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>

const int MAX_HEADER_LENGTH = 8*1024;

MessageBuilderHttp::MessageBuilderHttp()
    : MessageBuilder()
    , _content_length(-1)
    , _content_pos(-1)
    , _mode(BodyTransferMode::UNKNOWN) {
}


bool MessageBuilderHttp::IsMessageCompleted() {
  if(!_data_size || (_mode == BodyTransferMode::UNKNOWN)) {
    return false;
  }
  if(_mode == BodyTransferMode::NONE) {
    return true;
  }
  if(_mode == BodyTransferMode::CONTENT_LENGTH) {
    return (_data_size >= _expected_data_size);
  }
  if(_mode == BodyTransferMode::CHUNKED) {
    if(_data_size > (uint32_t)_content_pos) {
      return(!std::string((const char*)_data.get() + _data_size -4, 4).compare("\r\n\r\n"));
    }
  }
  return false;
}


void MessageBuilderHttp::Check(std::vector<std::shared_ptr<Message> >& out_msgs) {
  MaybeGetHeaderData();

  while(IsMessageCompleted()) {
    out_msgs.push_back(CreateMessage());
    MaybeGetHeaderData();
  }
}

void MessageBuilderHttp::MaybeGetHeaderData() {
  if(_header) {
    return;
  }

  if(_data_size > 0) {

    if(_data_size > MAX_HEADER_LENGTH) {
      DLOG(error, "MessageBuilderHttp : Max header length exceeded");
      _data_size = 0;
      _data_size_cap = 0;
      _data = nullptr;
      _expected_data_size = 0;
      _content_length = -1;
      _header = nullptr;
      return;
    }

    auto data_str = std::string((const char*)_data.get(), _data_size);
    auto header_end = data_str.find("\r\n\r\n");
    if( header_end == std::string::npos) {
      return;
    }

    _content_pos = (int)header_end + 4;
    _content_length = 0;
    std::string header_str = std::string((const char*)_data.get(), header_end);
    _header = HttpHeader::Parse(header_str);
    if(_header) {
      std::string content_len_val_str;
      std::string transfer_encoding;
      if(_header->GetFieldValue(HttpHeaderField::CONTENT_LENGTH, content_len_val_str)) {
        if(!StringUtils::ToInt(content_len_val_str, _content_length)) {
          DLOG(error, "MessageBuilderHttp : Can't parse content length : {}", content_len_val_str);
          _content_length = 0;
        }
        _expected_data_size = _content_pos + _content_length;
        _mode = BodyTransferMode::CONTENT_LENGTH;
      } else if(_header->GetFieldValue(HttpHeaderField::TRANSFER_ENCODING, transfer_encoding)) {
        if(!transfer_encoding.compare("chunked")) {
          _mode = BodyTransferMode::CHUNKED;
        }
      } else {
        _mode = BodyTransferMode::NONE;
        _expected_data_size = _content_pos;
      }
    }
  }
}

std::shared_ptr<Message> MessageBuilderHttp::CreateMessage() {
  uint32_t body_size = 0;
  if(_mode == BodyTransferMode::CONTENT_LENGTH) {
    body_size = _expected_data_size - _content_pos;
  } else if(_mode == BodyTransferMode::CHUNKED){
    body_size = _data_size - _content_pos;
  }

  std::shared_ptr<HttpMessage> msg = std::make_shared<HttpMessage>(body_size,
                                                                   _data,
                                                                   _content_pos,
                                                                   true,
                                                                   _header);

  if((_data_size > _expected_data_size) &&
      (_mode != BodyTransferMode::CHUNKED)) {
    _data_size_cap = (_data_size - _expected_data_size)*2;
    std::shared_ptr<unsigned char> new_data(new unsigned char[_data_size_cap],
      std::default_delete<unsigned char[]>());
    std::memcpy(new_data.get(), (void*)(_data.get() + _expected_data_size), _data_size - _expected_data_size);
    _data_size = _data_size - _expected_data_size;
    _data = new_data;
    _expected_data_size = 0;
    _content_length = -1;
    _header = nullptr;
  }
  else {
    _data_size = 0;
    _data_size_cap = 0;
    _data = nullptr;
  }
  _mode = BodyTransferMode::UNKNOWN;
  _expected_data_size = 0;
  _content_length = -1;
  _header = nullptr;
  return msg;
}
