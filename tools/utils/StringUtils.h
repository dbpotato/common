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

#include <algorithm>
#include <locale>
#include <sstream>
#include <string>
#include <vector>


namespace StringUtils {
  static std::string Lowercase(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
  };

  static std::string Uppercase(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
  };

  static std::string TrimWhitespace(std::string str) {
    auto it = std::find_if(str.begin(), str.end(), [](char ch){
        return !std::isspace<char>(ch , std::locale::classic()); });
    str.erase( str.begin(), it);

    auto it_rev =  std::find_if(str.rbegin(), str.rend(), [](char ch){
        return !std::isspace<char>(ch , std::locale::classic()); });
    str.erase( it_rev.base(), str.end());

    return str;
  };

  static std::vector<std::string> Split(const std::string &str, const std::string &delim, size_t max_splits = 0) {
    std::string item;
    std::vector<std::string> elements;
    size_t last_pos = 0;

    size_t pos = str.find(delim, last_pos);
    while(pos != std::string::npos) {
      if(pos == last_pos) {
        last_pos = pos + delim.length();
        pos = str.find(delim, last_pos);
        continue;
      }
      if(max_splits && elements.size() >= max_splits) {
        elements.push_back(str.substr(last_pos));
        return elements;
      }
      elements.push_back(str.substr(last_pos, pos - last_pos));
      last_pos = pos + delim.length();
      pos = str.find(delim, last_pos);
    }
    if(last_pos < str.length() -1) {
       elements.push_back(str.substr(last_pos));
    }
    return elements;
  };
};
