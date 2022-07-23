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

#pragma once

#include <memory>
#include <unistd.h>

const int MESSAGE_HEADER_LENGTH = 5;
const int MESSAGE_HEADER_TYPE_LENGTH = 1;
const int MESSAGE_HEADER_SIZE_LENGTH = 4;

class Message;

class MessageWriteRequest {
public:
  std::shared_ptr<Message> _msg;
  size_t _write_offset;
  MessageWriteRequest(std::shared_ptr<Message> msg);
};

class Message : public std::enable_shared_from_this<Message> {
public:
  uint8_t _type;
  uint32_t _size;
  std::shared_ptr<unsigned char> _data;
  bool _is_raw;

  Message(uint32_t size, const void* data);
  Message(uint32_t size, std::shared_ptr<unsigned char> data, uint32_t offset = 0, bool copy = false);
  Message(const std::string& str);

  Message(uint8_t type);
  Message(uint8_t type, uint32_t size, const void* data);
  Message(uint8_t type, uint32_t size,
          std::shared_ptr<unsigned char> data,
          uint32_t offset = 0,
          bool copy = false);

  Message(uint8_t type, const std::string& str);

  bool CopyData(void* dest, uint32_t size, uint32_t offset = 0);
  std::string ToString(uint32_t offset = 0);
  ssize_t WriteInto(int file_desc, ssize_t offset = 0);
  virtual std::shared_ptr<Message> ConvertToBaseMessage();
};
