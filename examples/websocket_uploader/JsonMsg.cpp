/*
Copyright (c) 2024 Adam Kaniewski

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

#include "Logger.h"
#include "JsonMsg.h"
#include "Data.h"


const std::string EMPTY_JSON_STR = "{}";


JsonMsg::JsonMsg()
    : _is_valid (false)
    , _type(Type::UNKNOWN) {
}

bool JsonMsg::Parse(const std::string& str) {
  try{
    _json = nlohmann::json::parse(str);
    _is_valid = true;
    TryDetectType();
  }
  catch (nlohmann::json::parse_error& e) {
    log()->info("parse error : {}", e.what());
    _is_valid = false;
  }
  return _is_valid;
}

std::string JsonMsg::ToString() {
  if(!_is_valid) {
    return {};
  }
  return _json.dump();
}

void JsonMsg::TryDetectType() {
  if(!_is_valid) {
    return;
  }

  std::string type = ValueToString("type");

  if(!type.compare("upload_req")) {
    _type = Type::UPLOAD_REQ;
  }
}

JsonMsg::Type JsonMsg::GetType() {
  return _type;
}

std::string JsonMsg::ValueToString(const std::string& key) {
  return ValueToString(_json, key);
}

std::string JsonMsg::ValueToString(const nlohmann::json& json, const std::string& key) {
  std::string result;
  auto it_value = json.find(key);
  if(it_value == json.end())
    return result;

  auto value = *it_value;
  result = value.get<std::string>();
  return result;
}

int JsonMsg::ValueToInt(const std::string& key) {
  return ValueToInt(_json, key);
}

int JsonMsg::ValueToInt(const nlohmann::json& json, const std::string& key) {
  int result = -1;
  auto it_value = json.find(key);
  if(it_value == json.end())
    return result;

  auto value = *it_value;
  result = value.get<int>();
  return result;
}

std::string JsonMsg::MakeOkResponseMsg() {
  auto jobj = nlohmann::json::object();
  jobj["type"] = "upload_resp";
  jobj["err"] = 0;
  jobj["err_msg"] = "";
  return jobj.dump();
}

std::string JsonMsg::MakeErrResponseMsg(int error, std::string& error_msg) {
  auto jobj = nlohmann::json::object();
  jobj["type"] = "upload_resp";
  jobj["err"] = error;
  jobj["err_msg"] = error_msg;
  return jobj.dump();
}

std::string JsonMsg::Empty() {
  return EMPTY_JSON_STR;
}
