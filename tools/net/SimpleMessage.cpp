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

#include "SimpleMessage.h"
#include "DataResource.h"
#include "Logger.h"


SimpleMessage::Header::Header(uint8_t type, uint32_t size)
    : _type(type)
    , _size(size) {
  _header_data = std::make_shared<Data>(sizeof(uint8_t), (const unsigned char*)&_type);
  _header_data->Add(std::move(Data(sizeof(uint32_t), (const unsigned char*)&_size)));
}

SimpleMessage::SimpleMessage(std::shared_ptr<Header> header, std::shared_ptr<DataResource> content)
    : _header(header)
    , _content(content) {
}

SimpleMessage::SimpleMessage(uint8_t type) {
  _header = std::make_shared<Header>(type, 0);
  _content = std::make_shared<DataResource>();
}

SimpleMessage::SimpleMessage(uint8_t type, std::shared_ptr<DataResource> content)
    : SimpleMessage(std::make_shared<Header>(type, content->GetSize()), content) {
}

SimpleMessage::SimpleMessage(uint8_t type, const std::string& msg) {
  _header = std::make_shared<Header>(type, msg.length());
  _content = std::make_shared<DataResource>(std::make_shared<Data>(msg));
}

std::shared_ptr<Data> SimpleMessage::GetDataSubset(size_t max_size, size_t offset) {
  return Message::CreateSubsetFromHeaderAndResource(_header->_header_data, _content, max_size, offset);
}




uint32_t SimpleMessageBuilder::AddDataToCurrentCut(std::shared_ptr<Data> data) {
  _resource->AddData(data);
  return _resource->GetSize();
}

bool SimpleMessageBuilder::FindCutHeader(std::shared_ptr<Data> data, uint32_t& out_expected_cut_size) {
  if(data->GetCurrentSize() < 5) {
    return true;
  }

  uint8_t type = 0;
  uint32_t size = 0;
  data->CopyTo(&type, 0, 1);
  data->CopyTo(&size, 1, 4);

  _header = std::make_shared<SimpleMessage::Header>(type, size);
  _resource = std::make_shared<DataResource>();
  _resource->SetExpectedSize(size);

  out_expected_cut_size = size;

  data->AddOffset(5);
  return true;
}

void SimpleMessageBuilder::FindCutFooter(std::shared_ptr<Data> data) {
  _messages_to_send.emplace_back(std::make_shared<SimpleMessage>(_header, _resource));
}

bool SimpleMessageBuilder::AddData(std::shared_ptr<Data> data, std::vector<std::shared_ptr<Message> >& out_msgs) {
  bool data_add_success = TapeCutter::AddData(data);

  if(!data_add_success) {
    DLOG(error, "SimpleMessageBuilder::AddData Failed");
    return false;
  }

  out_msgs.insert(out_msgs.end(), _messages_to_send.begin(), _messages_to_send.end());
  _messages_to_send.clear();
  return true;
}