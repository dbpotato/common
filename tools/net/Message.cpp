/*
Copyright (c) 2018 - 2022 Adam Kaniewski

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

#include "Message.h"

#include <string>
#include <cstring>
#include <unistd.h>


MessageWriteRequest::MessageWriteRequest(std::shared_ptr<Message> msg)
    : _msg(msg)
    , _write_offset(0) {
}


Message::Message(uint32_t size, const void* data)
  : _type(0)
  , _size(size)
  , _is_raw(true) {
  if(_size) {
    _data = std::shared_ptr<unsigned char>(new unsigned char[_size], std::default_delete<unsigned char[]>());
    std::memcpy(_data.get(), data, _size);
  }
}

Message::Message(uint32_t size, std::shared_ptr<unsigned char> data, uint32_t offset, bool copy)
  : _type(0)
  , _size(size)
  , _data(data)
  , _is_raw(true) {
  if(copy || offset) {
    _data = std::shared_ptr<unsigned char>(new unsigned char[_size], std::default_delete<unsigned char[]>());
    std::memcpy(_data.get(), data.get() + offset, _size);
  }
}

Message::Message(const std::string& str)
  : _type(0)
  , _size(str.length())
  , _is_raw(true) {
  if(_size) {
    _data = std::shared_ptr<unsigned char>(new unsigned char[_size], std::default_delete<unsigned char[]>());
    std::memcpy(_data.get(), str.c_str(), _size);
  }
}

Message::Message(uint8_t type)
  : _type(type)
  , _size(0)
  , _is_raw(false) {
}

Message::Message(uint8_t type, uint32_t size, const void* data)
    : Message(size, data) {
  _type = type;
  _is_raw = false;
}

Message::Message(uint8_t type, uint32_t size, std::shared_ptr<unsigned char> data, uint32_t offset, bool copy)
    : Message(size, data, offset, copy) {
  _type = type;
  _is_raw = false;
}

Message::Message(uint8_t type, const std::string& str)
    : Message(str) {
  _type = type;
  _is_raw = false;
}

std::shared_ptr<Message> Message::ConvertToBaseMessage() {
  return shared_from_this();
}

std::string Message::ToString(uint32_t offset) {
  std::string result;
  if(_size) {
      result = std::string((const char*)_data.get() + offset, _size - offset);
  }
  return result;
}

bool Message::CopyData(void* dest, uint32_t size, uint32_t offset) {
  if(_size < size + offset)
      return false;

  std::memcpy(dest, _data.get() + offset, size);
  return true;
}

ssize_t Message::WriteInto(int file_desc, ssize_t offset) {

  if(offset >= _size)
    return -1;

  ssize_t result = 0;

  if(!offset && !_is_raw) {
    uint8_t header[MESSAGE_HEADER_LENGTH];
    std::memcpy(header, &_type, sizeof(_type));
    std::memcpy(header + sizeof(_type), &_size, sizeof(_size));

    result = write(file_desc, (unsigned char*)header, MESSAGE_HEADER_LENGTH);
    if(result != MESSAGE_HEADER_LENGTH) {
      return -1;
    }
  }

  if(_size) {
    result = write(file_desc, _data.get() + offset, _size - offset);
    if (result == -1) {
      return -1;
    }
    else if(result != _size - offset) {
      return offset + result;
    }
  }

  return 0;
}
