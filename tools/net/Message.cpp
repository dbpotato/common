/*
Copyright (c) 2018 - 2025 Adam Kaniewski

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

#include "Message.h"
#include "Data.h"
#include "DataResource.h"

#include <string>
#include <cstring>
#include <unistd.h>

MessageWriteRequest::MessageWriteRequest(std::shared_ptr<Message> msg)
    : _msg(msg)
    , _write_offset(0)
    , _total_write(0) {
}

Message::Message() {
}

Message::Message(const std::string& str) {
  _data_resource = std::make_shared<DataResource>(std::make_shared<Data>(str));
};

Message::Message(std::shared_ptr<Data> data) {
  _data_resource = std::make_shared<DataResource>(data);
};

Message::Message(std::shared_ptr<DataResource> data_resource)
    : _data_resource(data_resource) {
}

std::shared_ptr<DataResource> Message::GetDataResource() {
  return _data_resource;
}

std::shared_ptr<Data> Message::GetDataSubset(size_t max_size, size_t offset) {
  return Message::CreateSubsetFromHeaderAndResource(nullptr, _data_resource, max_size, offset);
}

std::shared_ptr<Data> Message::CreateSubsetFromHeaderAndResource(std::shared_ptr<Data> header,
                                                std::shared_ptr<DataResource> resource,
                                                size_t max_size,
                                                size_t offset) {
  std::shared_ptr<Data> result = std::make_shared<Data>();
  size_t header_data_size = 0;
  size_t header_current_size = 0;

  if(header) {
    header_current_size = header->GetCurrentSize();
    if((uint64_t)offset < header_current_size) {
      auto buff = std::shared_ptr<unsigned char>(new unsigned char[max_size],
                                                std::default_delete<unsigned char[]>());
      result = std::make_shared<Data>((uint64_t)max_size, buff);
      header_data_size = header_current_size - offset;
      if(header_data_size > max_size) {
        header_data_size = max_size;
      }
      std::memcpy(buff.get(), header->GetCurrentDataRaw(), header_data_size);
    }

    if(header_data_size == max_size) {
      return result;
    }

    if(!resource || !resource->GetSize()) {
      result->SetCurrentSize(header_data_size);
      return result;
    }
  }

  max_size -= header_data_size;

  uint64_t resource_offset = 0;
  if(offset > header_current_size) {
    resource_offset = offset - header_current_size;
  }

  uint64_t resource_cpy_size = resource->GetSize() - resource_offset;
  if(resource_cpy_size > max_size) {
    resource_cpy_size = max_size;
  }

  if(!resource->UseDriveCache() && !header_data_size) {
    result = Data::MakeShallowCopy(resource->GetMemCache());
    result->SetOffset(resource_offset);
    result->SetCurrentSize(resource_cpy_size);
  } else {
    if(!header_data_size) {
      auto buff = std::shared_ptr<unsigned char>(new unsigned char[resource_cpy_size], std::default_delete<unsigned char[]>());
      result = std::make_shared<Data>((uint64_t)resource_cpy_size, buff);
    } else {
      result->SetCurrentSize(header_data_size + resource_cpy_size);
    }
    resource->CopyToBuff(result->GetCurrentDataRaw() + header_data_size, resource_cpy_size, resource_offset);
  }

  return result;
}
