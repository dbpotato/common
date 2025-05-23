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

#pragma once

#include "Data.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>


class DataResource {
public:
  DataResource(bool enable_drive_cache = true);
  DataResource(std::shared_ptr<Data> data, bool enable_drive_cache = true);
  static std::shared_ptr<DataResource> CreateFromFile(std::string file_name);
  ~DataResource();
  bool AddData(std::shared_ptr<Data> data);
  bool AddData(std::shared_ptr<DataResource> resource);
  bool UseDriveCache();
  bool IsCompleted();
  uint64_t GetSize();
  uint64_t GetExpectedSize();
  void SetExpectedSize(uint64_t expected_size);
  std::shared_ptr<Data> GetMemCache();
  std::shared_ptr<Data> GetLastRecivedData();
  std::fstream& GetDriveCache();
  std::string GetDriveCacheFileName();
  bool SaveToFile(std::string file_name);
  bool SaveToFile(std::filesystem::path& path);
  void CopyToBuff(unsigned char* buff, size_t buff_size, size_t offset);
protected:
  void SetCompletedSize(uint64_t size);
private:
  bool WriteToDrive(std::shared_ptr<Data> data);
  uint64_t _loaded_size;
  uint64_t _expected_size;
  bool _enable_drive_cache;
  std::shared_ptr<Data> _last_received_data;
  std::shared_ptr<Data> _mem_cached_data;
  std::fstream _drive_cached_data;
  std::string _cache_file_name;
};