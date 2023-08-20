/*
Copyright (c) 2022 - 2023 Adam Kaniewski

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
#include "Data.h"
#include "DataResource.h"
#include "MimeTypeFinder.h"

HttpMessage::HttpMessage(int status_code,
                         const std::string& body_text)
    : Message() {
  CreateHeader(status_code, body_text.length());
  _resource = std::make_shared<DataResource>(std::make_shared<Data>(body_text));
}

HttpMessage::HttpMessage(HttpHeaderMethod::Type method,
                         const std::string& request,
                         const std::string& body_text)
    : Message(body_text) {
  CreateHeader(method, request, body_text.length());
}

HttpMessage::HttpMessage(std::shared_ptr<HttpHeader> header,
                         std::shared_ptr<DataResource> resource)
    : _header(header)
    , _resource(resource) {
}

void HttpMessage::CreateHeader(int status_code, uint32_t body_size) {
  _header = std::make_shared<HttpHeader>(HttpHeaderProtocol::HTTP_1_1, status_code);
  _header->SetField(HttpHeaderField::CONTENT_LENGTH, std::to_string(body_size));
}

void HttpMessage::CreateHeader(HttpHeaderMethod::Type method, const std::string& request, uint32_t body_size) {
  _header = std::make_shared<HttpHeader>(HttpHeaderProtocol::HTTP_1_1, method, request);
  if(body_size) {
    _header->SetField(HttpHeaderField::CONTENT_LENGTH, std::to_string(body_size));
  }
}

std::shared_ptr<HttpMessage> HttpMessage::CreateFromFile(const std::string& file_path) {
  auto data = DataResource::CreateFromFile(file_path);
  if(!data) {
    return nullptr;
  }
  auto header = std::make_shared<HttpHeader>(HttpHeaderProtocol::HTTP_1_1, 200);
  header->SetField(HttpHeaderField::CONTENT_TYPE, MimeTypeFinder::Find(file_path));
  header->SetField(HttpHeaderField::CONTENT_LENGTH, std::to_string(data->GetSize()));
  auto msg = std::make_shared<HttpMessage>(header, data);
  return msg;
}

std::shared_ptr<Data> HttpMessage::GetDataSubset(size_t max_size, size_t offset) {
  if(!_header_str_data) {
    _header_str_data = std::make_shared<Data>(_header->ToString());
  }
  return CreateSubsetFromHeaderAndResource(_header_str_data, _resource, max_size, offset);
}

std::shared_ptr<HttpHeader> HttpMessage::GetHeader() {
  return _header;
}

std::shared_ptr<DataResource> HttpMessage::GetResource() {
  return _resource;
}
