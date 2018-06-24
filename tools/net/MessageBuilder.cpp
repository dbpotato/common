/*
Copyright (c) 2018 Adam Kaniewski

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


#include "MessageBuilder.h"
#include "Message.h"
#include "Logger.h"

#include <cstring>

MessageBuilder::MessageBuilder()
  : _expected_data_size(0)
  , _data_size(0)
  , _data_size_cap(0)
  , _type(0) {
}

void MessageBuilder::MaybeResize(size_t add_size) {
  if(_data_size + add_size > _data_size_cap) {
     std::shared_ptr<unsigned char> new_data(new unsigned char[(_data_size + add_size)*2],
         std::default_delete<unsigned char[]>());
     std::memcpy(new_data.get(), (void*)(_data.get()), _data_size);
     _data_size_cap = (_data_size + add_size)*2;
     _data = new_data;
  }
}

void MessageBuilder::AddData(Data data, std::vector<std::shared_ptr<Message> >& out_msgs) {

  if(!data._size)
    return;

  if(!_data_size) {
    _data = data._data;
    _data_size = data._size;
  }
  else {
   MaybeResize(data._size);
   std::memcpy(_data.get() + _data_size, (void*)data._data.get(), data._size);
   _data_size += data._size;
  }

  Check(out_msgs);
}

void MessageBuilder::Check(std::vector<std::shared_ptr<Message> >& out_msgs) {

  if(_data_size < 5)
    return;

  MaybeGetHeaderData();

  while(_data_size >=5 && _data_size >= _expected_data_size) {
    out_msgs.push_back(CreateMessage());
    MaybeGetHeaderData();
  }
}

void MessageBuilder::MaybeGetHeaderData() {
  if(_data_size >= 5 && !_expected_data_size) {
    _type = _data.get()[0];
    std::memcpy(&_expected_data_size, (void*)(_data.get() + 1), 4);
    _expected_data_size += 5;
  }
}

std::shared_ptr<Message> MessageBuilder::CreateMessage() {
  std::shared_ptr<Message> msg = std::make_shared<Message>(_type, _expected_data_size - 5, _data, 5, true);
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
