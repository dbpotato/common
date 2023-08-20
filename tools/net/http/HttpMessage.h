/*
Copyright (c) 2022 -2023 Adam Kaniewski

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

#include "HttpHeader.h"
#include "Message.h"

class Data;
class DataResource;

class HttpMessage : public Message {
public :

  HttpMessage(int status_code,
              const std::string& body_text = {});


  HttpMessage(HttpHeaderMethod::Type method,
              const std::string& request,
              const std::string& body_text = {});

  HttpMessage(std::shared_ptr<HttpHeader> header,
              std::shared_ptr<DataResource> resource);


  std::shared_ptr<HttpHeader> GetHeader();
  std::shared_ptr<DataResource> GetResource();
  static std::shared_ptr<HttpMessage> CreateFromFile(const std::string& file_path);
  std::shared_ptr<Data> GetDataSubset(size_t max_size, size_t offset) override;

private :
  void CreateHeader(int status_code, uint32_t body_size);
  void CreateHeader(HttpHeaderMethod::Type method, const std::string& request, uint32_t body_size);
  std::shared_ptr<HttpHeader> _header;
  std::shared_ptr<DataResource> _resource;
  std::shared_ptr<Data> _header_str_data;
};
