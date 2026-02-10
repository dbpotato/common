/*
Copyright (c) 2025 Adam Kaniewski

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

#include "DirectoryListing.h"
#include "Logger.h"

#include <chrono>
#include <cstring>
#include <algorithm>
#include <sys/stat.h>


void DirectoryLastModifiedTime(const std::filesystem::path& path, int64_t& time_sec_value) {
  struct stat info;
  time_sec_value = 0;

  struct stat dirInfo;
  if (stat(path.string().c_str(), &dirInfo) == 0) {
    time_sec_value = (int64_t)dirInfo.st_ctime;
  }

  std::error_code err;
  std::filesystem::directory_iterator dir_it(path, err);
  if(err) {
    return;
  }

  for(const auto& entry : dir_it) {
    bool regular_file = std::filesystem::is_regular_file(entry, err);
    if(!regular_file || err) {
      continue;
    }

    struct stat subInfo;
    if (stat(entry.path().string().c_str(), &subInfo) == 0) {
      if(subInfo.st_mtime > (int64_t)time_sec_value) {
        time_sec_value = (int64_t)subInfo.st_mtime;
      }
    }
  }
}

bool FileLastModificationTime(const std::filesystem::path& path, int64_t& time_sec_value) {
  struct stat info;
  if(stat(path.string().c_str(), &info) == 0) {
    time_sec_value = (int64_t)info.st_mtime;
    return true;
  }
  return false;
}

std::shared_ptr<Data> DirectoryListing::SerializeDirectory(const std::string& dir_path) {
  std::shared_ptr<Data> data;

  std::vector<std::filesystem::path> files;
  std::vector<std::filesystem::path> directories;
  std::error_code err;
  std::filesystem::directory_iterator dir_it(dir_path, err);
  if(err) {
    return data;
  }

  for (const auto& entry : dir_it) {
    bool is_dir = std::filesystem::is_directory(entry, err);
    if(err) {
      continue;
    }
    if (is_dir) {
      directories.emplace_back(entry.path());
    } else {
      files.emplace_back(entry.path());
    }
  }

  std::sort(directories.begin(), directories.end());
  std::sort(files.begin(), files.end());

  std::vector<FileInfo> info_vec;
  PathToFileInfoVec(directories, info_vec);
  PathToFileInfoVec(files, info_vec);

  uint32_t needed_size = info_vec.size() * (2 + 8 + 8 + 1);
  for (const auto& file_info : info_vec) {
    needed_size += file_info.name_length;
  }

  data = std::make_shared<Data>(needed_size);

  for (const auto& file_info : info_vec) {
    data->Add(2, (unsigned char*)&file_info.name_length);
    data->Add(file_info.name_length, (unsigned char*)file_info.name.c_str());
    data->Add(8, (unsigned char*)&file_info.size);
    data->Add(8, (unsigned char*)&file_info.last_modified);
    data->Add(1, (unsigned char*)&file_info.is_directory);
  }
  return data;
}

void DirectoryListing::PathToFileInfoVec(std::vector<std::filesystem::path>& paths, std::vector<FileInfo>& out_result) {
  std::error_code err;
  bool is_dir = false;
  bool is_regular_file = false;

  for (const auto& entry : paths) {
    FileInfo file_info;
    std::string entry_name = entry.filename().string();
    file_info.name = entry_name;
    file_info.name_length = (uint16_t)entry_name.length();
    file_info.is_directory = false;
    file_info.size = 0;

    is_dir = std::filesystem::is_directory(entry, err);
    if(err) {
      continue;
    }

    if(!is_dir) {
      is_regular_file = std::filesystem::is_regular_file(entry, err);
      if(err) {
        continue;
      }
      if(is_regular_file) {
        file_info.size = (uint64_t)std::filesystem::file_size(entry, err);
        if(err) {
          continue;
        }
        if(!FileLastModificationTime(entry, file_info.last_modified)) {
          DLOG(error, "File {}, time not valid", entry_name);
        }
      }
    } else {
      file_info.is_directory = true;
      DirectoryLastModifiedTime(entry, file_info.last_modified);
    }
    out_result.push_back(file_info);
  }
}

 bool DirectoryListing::DeserializeDirectory(std::shared_ptr<Data> data, std::vector<DirectoryListing::FileInfo>& out_files) {
  while (data->GetCurrentSize()) {
    FileInfo file_info;
    if(!data->CopyTo(&file_info.name_length, 0, 2)) {
      return false;
    }
    data->AddOffset(2);

    file_info.name.resize(file_info.name_length);
    if(!data->CopyTo(&file_info.name[0], 0, file_info.name_length)) {
      return false;
    }
    data->AddOffset(file_info.name_length);

    if(!data->CopyTo(&file_info.size, 0, 8)) {
      return false;
    }
    data->AddOffset(8);

    if(!data->CopyTo(&file_info.last_modified, 0, 8)) {
      return false;
    }
    data->AddOffset(8);

    if(!data->CopyTo(&file_info.is_directory, 0, 1)) {
      return false;
    }
    data->AddOffset(1);

    out_files.push_back(file_info);
  }
  return true;
}