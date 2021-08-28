/*
Copyright (c) 2021 Adam Kaniewski

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
#include "Message.h"
#include "Logger.h"

#include <cstring>
#include <algorithm>
#include <sstream>
#include <string>


MessageBuilderHttp::MessageBuilderHttp()
    : MessageBuilder()
    , _content_lenght(-1)
    , _content_pos(-1) {
}

void MessageBuilderHttp::Check(std::vector<std::shared_ptr<Message> >& out_msgs) {
  MaybeGetHeaderData();

  while(_data_size && _data_size >= _expected_data_size ) {
    out_msgs.push_back(CreateMessage());
    MaybeGetHeaderData();
  }
}

void MessageBuilderHttp::MaybeGetHeaderData() {
  if(_content_lenght == -1 && _data_size > 0) {
    auto data_str = std::string((const char*)_data.get(), _data_size);
    auto header_end = data_str.find("\r\n\r\n");
    if( header_end == std::string::npos) {
      _data = nullptr;
      _data_size = 0;
      _expected_data_size = 0;
      return;
    }

    _content_pos = (int)header_end + 4;
    auto header = std::string((const char*)_data.get(), header_end);

    std::transform(header.begin(), header.end(), header.begin(), [](unsigned char c){
      return std::tolower(c);
    });

    auto content_str_pos = header.find("content-length:");
    if(content_str_pos == std::string::npos) {
      _content_lenght = 0;
      _expected_data_size = _content_pos;
    } else {
      auto content_str_pos_end = header.find("\r\n", content_str_pos);
      auto content_len_val_str =  std::string((const char*)_data.get() + content_str_pos + 15, header_end - content_str_pos_end);
      _content_lenght =  std::stoi(content_len_val_str);
      _expected_data_size = _content_pos + _content_lenght;
    }
  }
}

std::shared_ptr<Message> MessageBuilderHttp::CreateMessage() {
  std::shared_ptr<Message> msg = std::make_shared<Message>(_expected_data_size, _data, 0, true);
  if(_data_size > _expected_data_size) {
    _data_size_cap = (_data_size - _expected_data_size)*2;
    std::shared_ptr<unsigned char> new_data(new unsigned char[_data_size_cap],
      std::default_delete<unsigned char[]>());
    std::memcpy(new_data.get(), (void*)(_data.get() + _expected_data_size), _data_size - _expected_data_size);
    _data_size = _data_size - _expected_data_size;
    _data = new_data;
    _expected_data_size = 0;
    _content_lenght = -1;
  }
  else {
    _data_size = 0;
    _data_size_cap = 0;
    _data = nullptr;
    _expected_data_size = 0;
    _content_lenght = -1;
  }
  return msg;
}
