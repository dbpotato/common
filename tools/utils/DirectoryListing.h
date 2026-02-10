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

#pragma once

#include <vector>
#include <filesystem>
#include <memory>

#include "Data.h"


namespace DirectoryListing {

struct FileInfo {
  uint16_t name_length;
  std::string name;
  uint64_t size;
  int64_t last_modified;
  bool is_directory;
};

std::shared_ptr<Data> SerializeDirectory(const std::string& dir_path);
bool DeserializeDirectory(std::shared_ptr<Data> data, std::vector<DirectoryListing::FileInfo>& out_files);
void PathToFileInfoVec(std::vector<std::filesystem::path>& paths, std::vector<FileInfo>& out_result);

}; //namespace DirectoryListing