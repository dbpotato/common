/*
Copyright (c) 2022 Adam Kaniewski

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

#include <fstream>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>


namespace FileUtils {
  static bool FileExists(const std::string& file_path) {
    std::ifstream stream(file_path, std::ios::binary | std::ios::ate);
    if(!stream.is_open()) {
      return false;
    }
    stream.close();
    return true;
  }

  static bool ReadFile(const std::string& file_path,
                       std::shared_ptr<unsigned char>& out_data,
                       size_t& out_data_size) {
    bool result = true;
    std::ifstream stream(file_path, std::ios::binary | std::ios::ate);
    if(!stream.is_open()) {
      return false;
    }
    out_data_size = (size_t)stream.tellg();
    stream.seekg(0, std::ios::beg);
    out_data = std::shared_ptr<unsigned char>(new unsigned char[out_data_size], std::default_delete<unsigned char[]>());
    result = stream.read((char*)(out_data.get()), out_data_size).good();
    stream.close();
    return result;
  }

  static bool ReadFile(const std::string& file_path,
                       std::string& out_body) {
    bool result = true;
    std::shared_ptr<unsigned char> data;
    size_t data_size = 0;
    if(!ReadFile(file_path, data, data_size)){
      return false;
    }
    out_body = std::string((const char*)data.get(), data_size);
    return true;
  }

  static bool SaveFile(const std::string& file_path,
                       std::shared_ptr<unsigned char> data,
                       size_t data_size) {
    bool result = true;
    std::ofstream stream(file_path, std::ios::out | std::ios::binary);
    if(!stream.is_open()) {
      return false;
    }
    result = stream.write((char*)data.get(), data_size).good();
    stream.close();
    return result;
  }
};
