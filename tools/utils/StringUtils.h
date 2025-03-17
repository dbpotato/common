/*
Copyright (c) 2022 - 2025 Adam Kaniewski

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
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
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
    if(last_pos < str.length() || !str.length()) {
       elements.push_back(str.substr(last_pos));
    }
    return elements;
  }

  static std::string Replace(const std::string &str,
                            const std::string &old_string,
                            const std::string &new_string,
                            size_t max_replacements = 0) {
    size_t counter = 0;
    std::string result = str;
    size_t pos = result.find(old_string);
    while (pos != std::string::npos) {
      result.replace(pos, old_string.length(), new_string);
      if(++counter <= max_replacements || !max_replacements) {
        pos = result.find(old_string, pos + new_string.length());
      } else {
        pos = std::string::npos;
      }
    }
    return result;
  }

  static bool ToInt(const std::string &str, int& out_value, int base = 10) {
    try {
      out_value = std::stoi(str, nullptr, base);
    } catch (const std::invalid_argument& exc) {
      return false;
    } catch (const std::out_of_range & exc) {
      return false;
    }
    return true;
  }

  static bool ToInt(const std::string &str, long& out_value, int base = 10) {
    try {
      out_value = std::stol(str, nullptr, base);
    } catch (const std::invalid_argument& exc) {
      return false;
    } catch (const std::out_of_range & exc) {
      return false;
    }
    return true;
  }

  static bool ToInt(const std::string &str, long long &out_value, int base = 10) {
    try {
      out_value = std::stoll(str, nullptr, base);
    } catch (const std::invalid_argument& exc) {
      return false;
    } catch (const std::out_of_range & exc) {
      return false;
    }
    return true;
  }

  static bool ParseUrl(const std::string& url,
                      std::string& out_protocol,
                      std::string& out_host,
                      std::string& out_target,
                      int& out_port) {

    std::string host_target;
    std::vector<std::string> split = StringUtils::Split(url, "://", 1);

    if(split.size() == 2) {
      out_protocol = split.at(0);
      host_target = split.at(1);
    } else {
      host_target = url;
    }

    split = StringUtils::Split(host_target, "/", 1);
    out_host = split.at(0);

    std::vector<std::string> port_split = StringUtils::Split(out_host, ":", 1);
    if(port_split.size() == 2) {
      out_host = port_split.at(0);
      if(!StringUtils::ToInt(port_split.at(1), out_port)) {
        return false;
      }
    }

    if(split.size() == 2) {
      out_target += split.at(1);
    }
    return true;
  }

  static std::string UrlEncode(const std::string& url) {
    std::ostringstream encode;
    encode.fill('0');
    encode << std::hex;

    for (auto iter = url.begin(); iter != url.end(); ++iter) {
      auto c = (*iter);
      if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
        encode << c;
        continue;
      }

      encode << std::uppercase;
      encode << '%' << std::setw(2) << int((unsigned char) c);
      encode << std::nouppercase;
    }
    return encode.str();
  }

  static std::string UrlDecode(const std::string& url) {
    std::ostringstream decoded;
    size_t url_len = url.length();
    for (size_t pos = 0; pos < url_len; ++pos) {
      auto c = url.at(pos);
      switch(c) {
        case '%':
          if (pos < url_len -2 && url.at(pos + 1) && url.at(pos + 2)) {
            char hs[] {url.at(pos + 1), url.at(pos + 2)};
            decoded << static_cast<char>(strtol(hs, nullptr, 16));
            pos += 2;
          }
          break;
        case '+':
          decoded << ' ';
          break;
        default:
          decoded << c;
      }
    }
    return decoded.str();
  }

};//namespace StringUtils
