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

#include <cstring>
#include <cstdint>
#include <memory>
#include <string>

class Data {
public:
  Data();
  Data(const std::string& str);
  Data(uint32_t size, std::shared_ptr<unsigned char> data);
  Data(uint32_t size, const unsigned char* data);
  Data(uint32_t size);
  void Swap(std::shared_ptr<Data> data);
  static std::shared_ptr<Data> MakeShallowCopy(std::shared_ptr<Data> data);

  void Add(Data other_data);
  void Add(std::shared_ptr<Data> other_data);
  uint32_t GetTotalSize();
  uint32_t GetCurrentSize();
  uint32_t GetOffset();
  bool AddOffset(uint32_t offset);
  bool SetOffset(uint32_t offset);
  bool SetCurrentSize(uint32_t size);
  const std::shared_ptr<unsigned char> GetData();
  unsigned char* GetCurrentDataRaw();
  bool CopyTo(void* dest, uint32_t offset, uint32_t dest_size);
  std::string ToString();

protected:
  uint32_t _size;
  uint32_t _offset;
  std::shared_ptr<unsigned char> _data;
};
