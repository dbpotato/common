/*
Copyright (c) 2026 Adam Kaniewski

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

#include "CommandLineArgs.h"
#include "StringUtils.h"


CommandLineArgs::CommandLineArgs(int argc, char* argv[]) {
  Parse(argc, argv);
}

void CommandLineArgs::Parse(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);

    bool is_flag = (arg.size() > 2 && arg[0] == '-' && arg[1] == '-');
    if (!is_flag) {
      continue;
    }

    std::vector<std::string> key_val = StringUtils::Split(arg.substr(2), "=", 1);
    _flags[key_val.at(0)] = (key_val.size() > 1 ? key_val.at(1) : "");
  }
}

bool CommandLineArgs::HasFlag(const std::string& flag) const {
  return (_flags.find(flag) != _flags.end());
}

bool CommandLineArgs::GetFlagValueString(const std::string& flag, std::string& out_value) const {
  auto it = _flags.find(flag);
  if(it != _flags.end() && !it->second.empty()) {
    out_value = it->second;
    return true;
  }
  return false;
}


bool CommandLineArgs::GetFlagValueInt(const std::string& flag, int& out_value) const {
  int value = 0;
  std::string value_str;
  if(!GetFlagValueString(flag, value_str)) {
    return false;
  }

  if(!StringUtils::ToInt(value_str, value)) {
    return false;
  }
  out_value = value;
  return true;
}

bool CommandLineArgs::GetFlagValueFloat(const std::string& flag, float& out_value) const {
  float value = 0.0f;
  std::string value_str;
  if(!GetFlagValueString(flag, value_str)) {
    return false;
  }

  if(!StringUtils::ToFloat(value_str, value)) {
    return false;
  }
  out_value = value;
  return true;
}
