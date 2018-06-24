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

#pragma once

#include "Connection.h"

#include <memory>
#include <vector>

class Message;

class MessageBuilder {
public:
  MessageBuilder();
  void AddData(Data data, std::vector<std::shared_ptr<Message> >& out_msgs);
private:
  uint32_t _expected_data_size;
  uint32_t _data_size;
  uint32_t _data_size_cap;
  uint8_t _type;
  std::shared_ptr<unsigned char> _data;
  void MaybeResize(size_t add_size);
  void Check(std::vector<std::shared_ptr<Message> >& out_msgs);
  void MaybeGetHeaderData();
  std::shared_ptr<Message> CreateMessage();
};
