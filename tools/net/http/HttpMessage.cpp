/*
Copyright (c) 2022 Adam Kaniewski

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

#include "HttpHeader.h"
#include "HttpMessage.h"
#include "FileUtils.h"
#include "MimeTypeFinder.h"

#include <cstring>


HttpMessage::HttpMessage(int status_code,
                         const std::string& body_text)
    : Message(body_text) {
  CreateHeader(status_code, body_text.length());
}

HttpMessage::HttpMessage(int status_code,
                         uint32_t body_size,
                         const void* body_data)
    : Message(body_size, body_data) {
  CreateHeader(status_code, body_size);
}

HttpMessage::HttpMessage(HttpHeaderMethod::Type method,
                         const std::string& request,
                         const std::string& body_text)
    : Message(body_text) {
  CreateHeader(method, request, body_text.length());
}

HttpMessage::HttpMessage(HttpHeaderMethod::Type method,
                         const std::string& request,
                         uint32_t body_size,
                         const void* body_data)
    : Message(body_size, body_data) {
  CreateHeader(method, request, body_size);
}

HttpMessage::HttpMessage(uint32_t body_size,
                         std::shared_ptr<unsigned char> body_data,
                         uint32_t data_offset,
                         bool copy_data,
                         std::shared_ptr<HttpHeader> header)
    : Message(body_size, body_data, data_offset, copy_data)
    , _header(header) {
}

void HttpMessage::CreateHeader(int status_code, uint32_t body_size) {
  _header = std::make_shared<HttpHeader>(HttpHeaderProtocol::HTTP_1_1, status_code);
  _header->AddField(HttpHeaderField::CONTENT_LENGTH, std::to_string(body_size));
}

void HttpMessage::CreateHeader(HttpHeaderMethod::Type method, const std::string& request, uint32_t body_size) {
  _header = std::make_shared<HttpHeader>(HttpHeaderProtocol::HTTP_1_1, method, request);
  _header->AddField(HttpHeaderField::CONTENT_LENGTH, std::to_string(body_size));
}

std::shared_ptr<HttpMessage> HttpMessage::FromFile(const std::string& file_path) {
  std::shared_ptr<unsigned char> data;
  size_t data_size = 0;
  if(!FileUtils::ReadFile(file_path, data, data_size)) {
    return nullptr;
  }
  auto msg = std::make_shared<HttpMessage>(200, (uint32_t)data_size, data.get());
  msg->GetHeader()->AddField(HttpHeaderField::CONTENT_TYPE, MimeTypeFinder::Find(file_path));
  return msg;
}

std::shared_ptr<Message> HttpMessage::ConvertToBaseMessage() {
  if(!_header || !_header->IsValid())
    return {};

  std::string header_str = _header->ToString();
  size_t header_length = header_str.length();
  uint32_t new_msg_size = header_length + _size;
  std::shared_ptr<unsigned char> new_msg_data(new unsigned char[new_msg_size], std::default_delete<unsigned char[]>());

  std::memcpy(new_msg_data.get(), header_str.c_str(), header_length);
  if(_size) {
    std::memcpy(new_msg_data.get() + header_length, _data.get(), _size);
  }

  return std::make_shared<Message>(new_msg_size, new_msg_data);
}

std::shared_ptr<HttpHeader> HttpMessage::GetHeader() {
  return _header;
}
