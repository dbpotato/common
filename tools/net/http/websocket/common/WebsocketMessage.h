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

#pragma once

#include "Message.h"
#include "WebsocketHeader.h"
#include "Data.h"

class DataResource;

class WebsocketMessage : public Message {
public :
  WebsocketMessage(const std::string& str);
  WebsocketMessage(std::shared_ptr<WebsocketHeader> header, std::shared_ptr<DataResource> resource);

  static std::shared_ptr<WebsocketMessage> CreatePingMessage();
  static std::shared_ptr<WebsocketMessage> CreatePongMessage(std::shared_ptr<Data> ping_data);
  static std::shared_ptr<WebsocketMessage> CreateCloseMessage();

  std::shared_ptr<Data> GetDataSubset(size_t max_size, size_t offset) override;
  std::shared_ptr<WebsocketHeader> GetHeader();
  std::shared_ptr<DataResource> GetResource();

private :
  std::shared_ptr<WebsocketHeader> _header;
  std::shared_ptr<DataResource> _resource;
  std::shared_ptr<Data> _header_bin_data;
};
