/*
Copyright (c) 2023 - 2024 Adam Kaniewski

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


SimpleMessage::Header::Header(uint8_t type, uint64_t size)
    : _type(type)
    , _size(size) {
  size_t header_size = sizeof(type) + sizeof(size);
  _header_data = std::make_shared<Data>(header_size);
  _header_data->Add(sizeof(type), (const unsigned char*)&_type);
  _header_data->Add(sizeof(size), (const unsigned char*)&_size);
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




uint64_t SimpleMessageBuilder::AddDataToCurrentCut(std::shared_ptr<Data> data) {
  _resource->AddData(data);
  return _resource->GetSize();
}

bool SimpleMessageBuilder::FindCutHeader(std::shared_ptr<Data> data, uint64_t& out_expected_cut_size) {
  uint8_t type = 0;
  uint64_t size = 0;

  size_t header_size = sizeof(type) + sizeof(size);

  if(data->GetCurrentSize() < header_size) {
    return false;
  }


  data->CopyTo(&type, 0, sizeof(type));
  data->CopyTo(&size, sizeof(type), sizeof(size));

  _header = std::make_shared<SimpleMessage::Header>(type, size);
  _resource = std::make_shared<DataResource>();
  _resource->SetExpectedSize(size);

  out_expected_cut_size = size;

  data->AddOffset(header_size);
  return true;
}

void SimpleMessageBuilder::FindCutFooter(std::shared_ptr<Data> data) {
  _messages_to_send.emplace_back(std::make_shared<SimpleMessage>(_header, _resource));
}

bool SimpleMessageBuilder::OnDataRead(std::shared_ptr<Data> data, std::vector<std::shared_ptr<Message> >& out_msgs) {
  bool data_add_success = TapeCutter::AddData(data);

  if(!data_add_success) {
    DLOG(error, "OnDataRead Failed");
    return false;
  }

  out_msgs.insert(out_msgs.end(), _messages_to_send.begin(), _messages_to_send.end());
  _messages_to_send.clear();
  return true;
}