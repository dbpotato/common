/*
Copyright (c) 2023 Adam Kaniewski

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

#include "Message.h"
#include "TapeCutter.h"

#include <memory>
#include <unistd.h>
#include <vector>


class Data;
class DataResource;


class SimpleMessage : public Message {
public:
  class Header {
  public :
    Header(uint8_t type, uint64_t size);
    uint8_t _type;
    uint64_t _size;
    std::shared_ptr<Data> _header_data;
  };

  SimpleMessage(uint8_t type);
  SimpleMessage(uint8_t type, const std::string& msg);
  SimpleMessage(uint8_t type, std::shared_ptr<DataResource> content);
  SimpleMessage(std::shared_ptr<Header> header, std::shared_ptr<DataResource> content);

  std::shared_ptr<Header> GetHeader() {return _header;}
  std::shared_ptr<DataResource> GetContent() {return _content;}

  std::shared_ptr<Data> GetDataSubset(size_t max_size, size_t offset) override;

private:
  using Message::GetData;
  std::shared_ptr<Header> _header;
  std::shared_ptr<DataResource> _content;
};

class SimpleMessageBuilder : public MessageBuilder, public TapeCutter {
public :
  bool OnDataRead(std::shared_ptr<Data> data, std::vector<std::shared_ptr<Message> >& out_msgs) override;
  uint64_t AddDataToCurrentCut(std::shared_ptr<Data> data) override;
  bool FindCutHeader(std::shared_ptr<Data> data, uint64_t& out_expected_cut_size) override;
  void FindCutFooter(std::shared_ptr<Data> data) override;
private:
  std::shared_ptr<SimpleMessage::Header> _header;
  std::shared_ptr<DataResource> _resource;
  std::vector<std::shared_ptr<SimpleMessage>> _messages_to_send;
};