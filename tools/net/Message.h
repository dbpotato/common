/*
Copyright (c) 2018 - 2023 Adam Kaniewski

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
#include <vector>


class Data;
class DataResource;
class Message;

class MessageWriteRequest {
public:
  std::shared_ptr<Message> _msg;
  size_t _write_offset;
  uint32_t _total_write;
  MessageWriteRequest(std::shared_ptr<Message> msg);
};

class MessageBuilder {
public:
  virtual bool AddData(std::shared_ptr<Data> data, std::vector<std::shared_ptr<Message> >& out_msgs) = 0;
};

class Message : public std::enable_shared_from_this<Message> {
public:
  Message();
  Message(const std::string& str);
  Message(std::shared_ptr<Data> data);
  virtual std::shared_ptr<Data> GetDataSubset(size_t max_size, size_t offset);
  std::shared_ptr<Data> GetData();
protected:
  std::shared_ptr<Data> CreateSubsetFromHeaderAndResource(std::shared_ptr<Data> header,
                                                std::shared_ptr<DataResource> resource,
                                                size_t max_size,
                                                size_t offset);
  std::shared_ptr<Data> _data;
};
