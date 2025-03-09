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

#include "Data.h"
#include "Logger.h"


Data::Data()
    : _allocated_size(0)
    , _used_size(0)
    , _offset(0) {
}

Data::Data(const std::string& str)
    : _allocated_size(str.length())
    , _used_size(str.length())
    , _offset(0) {
  _data = std::shared_ptr<unsigned char>(new unsigned char[_allocated_size],
                                          std::default_delete<unsigned char[]>());
  std::memcpy(_data.get(), str.c_str(), _allocated_size);
}

Data::Data(uint64_t size, std::shared_ptr<unsigned char> data)
    : _allocated_size(size)
    , _used_size(size)
    , _offset(0)
    , _data(data) {
}

Data::Data(uint64_t size, const unsigned char* data)
    : _allocated_size(size)
    , _used_size(size)
    , _offset(0) {
  _data = std::shared_ptr<unsigned char>(new unsigned char[_allocated_size],
                                          std::default_delete<unsigned char[]>());
  std::memcpy(_data.get(), data, _allocated_size);
}

Data::Data(uint64_t size)
    : _allocated_size(size)
    , _used_size(0)
    , _offset(0) {
  _data = std::shared_ptr<unsigned char>(new unsigned char[_allocated_size],
                                          std::default_delete<unsigned char[]>());
}

void Data::Swap(std::shared_ptr<Data> data) {
  _allocated_size = data->_allocated_size;
  _used_size = data->_used_size;
  _offset = data->_offset;
  _data = data->_data;
}

std::shared_ptr<Data> Data::MakeShallowCopy(std::shared_ptr<Data> data) {
  std::shared_ptr<Data> result = std::make_shared<Data>();
  result->Swap(data);
  return result;
}

void Data::Add(std::shared_ptr<Data> other_data) {
  if(other_data) {
    Add(*other_data.get());
  }
}

void Data::Add(Data other_data) {
  Add(other_data.GetCurrentSize(), other_data.GetCurrentDataRaw());
}

void Data::Add(uint64_t other_data_size, const unsigned char* other_data) {
  uint64_t new_size = _used_size + other_data_size;
  if(!new_size) {
    return;
  }

  if(new_size > _allocated_size) {
    auto new_data = std::shared_ptr<unsigned char>(new unsigned char[new_size],
                                           std::default_delete<unsigned char[]>());
    if(_used_size) {
      std::memcpy(new_data.get(), _data.get(), _used_size);
    }
    _allocated_size = new_size;
    _data = new_data;
  }

  std::memcpy(_data.get() + _used_size,
              other_data,
              other_data_size);
  _used_size = new_size;
}

uint64_t Data::GetTotalSize() {
  return _used_size;
}

uint64_t Data::GetCurrentSize() {
  return _used_size - _offset;
}

uint64_t Data::GetOffset() {
  return _offset;
}

bool Data::AddOffset(uint64_t offset) {
  return SetOffset(_offset + offset);
}

bool Data::SetOffset(uint64_t offset) {
  if(offset > _used_size) {
    DLOG(error, "trying to set offset larger than data size");
    return false;
  }
  _offset = offset;
  return true;
}

bool Data::SetCurrentSize(uint64_t size) {
  if(size + _offset > _allocated_size) {
    DLOG(error, "trying to set size larger than allocated size");
    return false;
  }
  _used_size = size + _offset;
  return true;
}

const std::shared_ptr<unsigned char> Data::GetData() {
  return _data;
}

unsigned char* Data::GetCurrentDataRaw() {
  return _data.get() + _offset;
}

bool Data::CopyTo(void* dest, uint64_t offset, uint64_t dest_used_size) {
  if(GetCurrentSize() < dest_used_size + offset) {
    DLOG(error, "trying to copy more data than currently availabe");
    return false;
  }
  std::memcpy(dest, GetCurrentDataRaw() + offset, dest_used_size);
  return true;
}

std::string Data::ToString() {
  std::string result;
  if(GetCurrentSize()) {
    result = std::string((const char*)GetCurrentDataRaw(), GetCurrentSize());
  }
  return result;
}
