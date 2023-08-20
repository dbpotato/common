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

#include "Data.h"
#include "Logger.h"


Data::Data()
    : _size(0), _offset(0) {
}

Data::Data(const std::string& str)
    : _size(str.length())
    , _offset(0) {
  _data = std::shared_ptr<unsigned char>(new unsigned char[_size],
                                          std::default_delete<unsigned char[]>());
  std::memcpy(_data.get(), str.c_str(), _size);
}

Data::Data(uint32_t size, std::shared_ptr<unsigned char> data)
    : _size(size)
    , _offset(0)
    , _data(data) {
}

Data::Data(uint32_t size, const unsigned char* data)
      : _size(size)
      , _offset(0) {
  _data = std::shared_ptr<unsigned char>(new unsigned char[_size],
                                          std::default_delete<unsigned char[]>());
  std::memcpy(_data.get(), data, _size);
}

Data::Data(uint32_t size)
    : _size(size)
    , _offset(0) {
  _data = std::shared_ptr<unsigned char>(new unsigned char[_size],
                                          std::default_delete<unsigned char[]>());
}

void Data::Swap(std::shared_ptr<Data> data) {
  _size = data->_size;
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
  uint32_t total_size = GetTotalSize();
  uint32_t new_size = total_size + other_data.GetCurrentSize();
  if(!new_size) {
    return;
  }

  auto new_data = std::shared_ptr<unsigned char>(new unsigned char[new_size],
                                           std::default_delete<unsigned char[]>());

  std::memcpy(new_data.get(), _data.get(), _size);
  std::memcpy(new_data.get() + _size,
              other_data.GetCurrentDataRaw(),
              other_data.GetCurrentSize());
  _size = new_size;
  _data = new_data;
}

uint32_t Data::GetTotalSize() {
  return _size;
}

uint32_t Data::GetCurrentSize() {
  return _size - _offset;
}

uint32_t Data::GetOffset() {
  return _offset;
}

bool Data::AddOffset(uint32_t offset) {
  return SetOffset(_offset + offset);
}

bool Data::SetOffset(uint32_t offset) {
  if(offset > _size) {
    DLOG(error, "Data: trying to set offset larger than data size");
    return false;
  }
  _offset = offset;
  return true;
}

bool Data::SetCurrentSize(uint32_t size) {
  if(size + _offset > _size) {
    DLOG(error, "Data: trying to set size larger than original size");
    return false;
  }
  _size = size + _offset;
  return true;
}

const std::shared_ptr<unsigned char> Data::GetData() {
  return _data;
}

unsigned char* Data::GetCurrentDataRaw() {
  return _data.get() + _offset;
}

bool Data::CopyTo(void* dest, uint32_t offset, uint32_t dest_size) {
  if(GetCurrentSize() < dest_size + offset) {
    DLOG(error, "Data: trying to copy more data than currently availabe");
    return false;
  }
  std::memcpy(dest, GetCurrentDataRaw() + offset, dest_size);
  return true;
}

std::string Data::ToString() {
  std::string result;
  if(GetCurrentSize()) {
    result = std::string((const char*)GetCurrentDataRaw(), GetCurrentSize());
  }
  return result;
}
