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


#include "DataResource.h"
#include "FileUtils.h"

const uint64_t MAX_MEM_CACHE_SIZE = 1024*1024*4;


DataResource::DataResource(bool enable_drive_cache)
    : _loaded_size(0)
    , _expected_size(0)
    , _enable_drive_cache(enable_drive_cache) {
  _mem_cached_data = std::make_shared<Data>();
}

DataResource::DataResource(std::shared_ptr<Data> data, bool enable_drive_cache)
    : _loaded_size(data->GetCurrentSize())
    , _expected_size(data->GetCurrentSize())
    , _enable_drive_cache(enable_drive_cache)
    , _mem_cached_data(data) {
}

DataResource::~DataResource() {
  if(!_cache_file_name.empty() && FileUtils::FileExists(_cache_file_name)){
    FileUtils::DeleteFile(_cache_file_name);
  }
}

std::shared_ptr<DataResource> DataResource::CreateFromFile(std::string file_name) {
  std::shared_ptr<DataResource> result = std::make_shared<DataResource>();
  std::fstream& stream = result->GetDriveCache();

  stream.open(file_name, std::ios::binary | std::ios::in | std::ios::ate);
  if(!stream.is_open()) {
    return nullptr;
  }

  stream.seekg(0, std::ios::end);
  auto stream_end = stream.tellg();
  if(stream_end == -1) {
    return nullptr;
  }
  uint64_t size = (uint64_t)stream_end;
  stream.seekg(0, std::ios::beg);
  result->SetCompletedSize(size);

  return result;
}

void DataResource::SetCompletedSize(uint64_t size) {
  _loaded_size = _expected_size = size;
}

bool DataResource::IsCompleted() {
  return (_loaded_size == _expected_size);
}

uint64_t DataResource::GetSize() {
  return _loaded_size;
}

uint64_t DataResource::GetExpectedSize() {
  return _expected_size;
}

void DataResource::SetExpectedSize(uint64_t expected_size) {
  _expected_size = expected_size;
}

bool DataResource::UseDriveCache() {
  return _drive_cached_data.is_open();
}

bool DataResource::AddData(std::shared_ptr<Data> data) {
  _last_received_data = Data::MakeShallowCopy(data);
  _loaded_size += data->GetCurrentSize();

  if(UseDriveCache()) {
    return WriteToDrive(data);
  } else if(_mem_cached_data->GetCurrentSize() + data->GetCurrentSize() > MAX_MEM_CACHE_SIZE) {
    if(!_enable_drive_cache) {
      return true;
    }

    _cache_file_name = FileUtils::CreateTempFileName();
    if(_cache_file_name.empty()){
      return false;
    }

    bool res = false;
    if(_mem_cached_data->GetCurrentSize()) {
      res = WriteToDrive(_mem_cached_data);
      _mem_cached_data = std::make_shared<Data>();
      if(res) {
        res = WriteToDrive(data);
      }
      return res;
    } else {
      return WriteToDrive(data);
    }
  } else {
    _mem_cached_data->Add(data);
  }
  return true;
}

bool DataResource::AddData(std::shared_ptr<DataResource> resource) {
  if(resource->UseDriveCache()) {
    if(!UseDriveCache()) {
      _cache_file_name = FileUtils::CreateTempFileName();
      if(_cache_file_name.empty()){
        return false;
      }
      if(!WriteToDrive(_mem_cached_data)){
        return false;
      }
    }

    _loaded_size += resource->GetSize();

    std::fstream& res_stream = resource->GetDriveCache();
    res_stream.seekg(0, res_stream.beg);

    size_t buff_length = 1024*64;
    char* buff = new char[buff_length];

    do {
      size_t read_len = buff_length;
      res_stream.read(buff, buff_length);
      if(!res_stream) {
        read_len = res_stream.gcount();
      }
      _drive_cached_data.write(buff, read_len);
    } while(res_stream);

    delete[] buff;

  } else {
    AddData(resource->GetMemCache());
  }
  return true;
}

bool DataResource::WriteToDrive(std::shared_ptr<Data> data) {
  if(!_drive_cached_data.is_open()) {
    _drive_cached_data.open(_cache_file_name, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    if(!_drive_cached_data.is_open()) {
      return false;
    }
  }

  if(!_drive_cached_data.write((const char*)data->GetCurrentDataRaw(), data->GetCurrentSize())) {
    return false;
  }
  return true;
}

std::shared_ptr<Data> DataResource::GetMemCache() {
  return _mem_cached_data;
}

std::shared_ptr<Data> DataResource::GetLastRecivedData() {
  return _last_received_data;
}

std::fstream& DataResource::GetDriveCache() {
  return _drive_cached_data;
}

std::string DataResource::GetDriveCacheFileName() {
  return _cache_file_name;
}

bool DataResource::SaveToFile(std::filesystem::path& path) {
  return SaveToFile(path.string());
}

bool DataResource::SaveToFile(std::string file_name) {
  if(UseDriveCache()) {
    _drive_cached_data.flush();
    return FileUtils::MoveFile(_cache_file_name, file_name);
  } else {
    return FileUtils::SaveFile(file_name, _mem_cached_data);
  }
  return true;
}

void DataResource::CopyToBuff(unsigned char* buff, size_t buff_size, size_t offset) {
  if(UseDriveCache()) {
    _drive_cached_data.seekg(offset);
    _drive_cached_data.read((char*)buff, buff_size); //TODO return val check
  } else {
    std::memcpy(buff, _mem_cached_data->GetCurrentDataRaw() + offset, buff_size);
  }
}
