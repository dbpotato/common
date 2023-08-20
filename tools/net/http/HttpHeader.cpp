/*
Copyright (c) 2022 -2023 Adam Kaniewski

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

#include "HttpHeader.h"
#include "Logger.h"
#include "StringUtils.h"

#include <cstdlib>
#include <sstream>
#include <vector>


const std::string INVALID_HEADER_STR = "Invalid Header";

HttpHeader::HttpHeader()
    : _protocol(HttpHeaderProtocol::Type::UNKNOWN_TYPE)
    , _method(HttpHeaderMethod::Type::UNKNOWN_TYPE)
    , _status_code(0)
    , _was_received(false)
    , _is_message_completed(false)
    , _loaded_data_size(0)
    , _expected_data_size(0) {
}

HttpHeader::HttpHeader(HttpHeaderProtocol::Type protocol, int status_code)
    : HttpHeader() {
  _protocol = protocol;
  _status_code = status_code;
}

HttpHeader::HttpHeader(HttpHeaderProtocol::Type protocol, HttpHeaderMethod::Type method, const std::string& request)
    : HttpHeader() {
  _protocol = protocol;
  _method = method;
  _request_target = request;
}

void HttpHeader::SetField(HttpHeaderField::Type type, const std::string& value) {
  auto it = _fields.find(type);
  if(it != _fields.end()) {
    it->second = value;
  } else {
    _fields.insert(std::make_pair(type, value));
  }
}

void HttpHeader::SetField(const std::string& type, const std::string& value) {
  auto it = _unknown_fields.find(type);
  if(it != _unknown_fields.end()) {
    it->second = value;
  } else {
    _unknown_fields.insert(std::make_pair(type, value));
  }
}

void HttpHeader::RemoveField(HttpHeaderField::Type type) {
  _fields.erase(type);
}

void HttpHeader::RemoveField(const std::string& type) {
  _unknown_fields.erase(type);
}

bool HttpHeader::HasField(HttpHeaderField::Type type) {
  auto it = _fields.find(type);
  return (it != _fields.end());
}

bool HttpHeader::GetFieldValue(HttpHeaderField::Type type, std::string& out_value) {
    auto it = _fields.find(type);
    if(it == _fields.end()) {
      return false;
    }
    out_value = it->second;
    return true;
}

const std::map<std::string, std::string>& HttpHeader::GetUnknownFields() {
  return _unknown_fields;
}

bool HttpHeader::WasReceived() {
  return _was_received;
}
void HttpHeader::SetReceived () {
  _was_received = true;
}

bool HttpHeader::IsMessageCompleted() {
  return _is_message_completed;
}

void HttpHeader::SetMessageCompleted() {
  _is_message_completed = true;
}

void HttpHeader::AddLoadedDataSize(uint32_t data_size) {
  _loaded_data_size += data_size;
}

void HttpHeader::SetLoadedDataSize(uint32_t data_size) {
  _loaded_data_size = data_size;
}

uint32_t HttpHeader::GetLoadedDataSize() {
  return _loaded_data_size;
}

void HttpHeader::SetExpectedDataSize(uint32_t data_size) {
  _expected_data_size = data_size;
}

uint32_t HttpHeader::GetExpectedDataSize() {
  return _expected_data_size;
}

std::shared_ptr<HttpHeader> HttpHeader::Parse(const std::string& header_str) {
  std::string line;
  std::string key;
  std::string value;
  HttpHeaderField::Type key_type;

  std::shared_ptr<HttpHeader> new_header;
  new_header.reset(new HttpHeader());

  std::vector<std::string> header_lines = StringUtils::Split(header_str, "\r\n");
  if(!header_lines.size()) {
    DLOG(warn, "HttpHeader::Parse : empty header");
    return {};
  }

  if(!new_header->ParseType(header_lines.at(0))) {
    DLOG(warn,"HttpHeader::Parse : failed to parse message type : {}", header_lines.at(0));
    return {};
  }

  for(size_t i = 1; i < header_lines.size(); ++i) {
    if(!new_header->KeyValSplit(header_lines.at(i), key, value)) {
      DLOG(warn, "HttpHeader::Parse : failed to split field line :{}", header_lines.at(i));
      continue;
    }
    if(HttpHeaderField::GetTypeFromString(key, key_type)) {
      new_header->SetField(key_type, value);
    } else {
      new_header->SetField(key, value);
    }
  }

  std::string content_len_val_str;
  if(new_header->GetFieldValue(HttpHeaderField::CONTENT_LENGTH, content_len_val_str)) {
    int value;
    if(StringUtils::ToInt(content_len_val_str, value)) {
      new_header->SetExpectedDataSize((uint32_t)value);
    }
  }
  return new_header;
}

bool HttpHeader::ParseType(const std::string& header_first_line) {
  std::vector<std::string> elements = StringUtils::Split(header_first_line, " ");
  if(elements.size() < 2) {
    return false;
  }

  if(HttpHeaderProtocol::GetTypeFromString(elements.at(0), _protocol)) {
    if(!ParseStatusCode(elements.at(1))) {
      return false;
    }
  } else if (HttpHeaderMethod::GetTypeFromString(elements.at(0), _method)) {
    if(elements.size() != 3) {
      return false;
    }
    if(!HttpHeaderProtocol::GetTypeFromString(elements.at(2), _protocol)) {
      return false;
    }
    _request_target = elements.at(1);
  } else {
    return false;
  }
  return true;
}

bool HttpHeader::ParseStatusCode(const std::string& code_str) {
  std::string status_str;
  _status_code = std::atoi(code_str.c_str());
  if(!HttpHeaderStatus::GetStringFromCode(_status_code, status_str)) {
    return false;
  }
  return true;
}

bool HttpHeader::KeyValSplit(const std::string& header_line, std::string& key, std::string& value) {
  std::vector<std::string> key_val = StringUtils::Split(header_line, ":", 1);
  if(key_val.size() != 2) {
    return false;
  }
  key = StringUtils::TrimWhitespace(key_val.at(0));
  value = StringUtils::TrimWhitespace(key_val.at(1));
  return true;
}


bool HttpHeader::IsValid() {
  if(_protocol == HttpHeaderProtocol::UNKNOWN_TYPE) {
    return false;
  }

  if((_status_code > 0) && HttpHeaderStatus::int_to_status_str.find(_status_code) == HttpHeaderStatus::int_to_status_str.end()) {
    return false;
  } else {
    return true;
  }

  if(_method == HttpHeaderMethod::UNKNOWN_TYPE || _request_target.empty()) {
    return false;
  }

  return true;
}

std::string HttpHeader::ToString() {
  std::stringstream str_stream;
  std::string str_status_code;

  if(_method != HttpHeaderMethod::Type::UNKNOWN_TYPE) {
    str_stream << HttpHeaderMethod::GetStringFromType(_method) << " "
               << _request_target << " "
               << HttpHeaderProtocol::GetStringFromType(_protocol)  << "\r\n";
  } else if(HttpHeaderStatus::GetStringFromCode(_status_code, str_status_code)) {
    str_stream << HttpHeaderProtocol::GetStringFromType(_protocol) << " "
               << _status_code << " "
               << str_status_code << "\r\n";
  } else {
    DLOG(warn, "HttpHeader : failed to convert to string");
    return INVALID_HEADER_STR;
  }

  for(auto field_val : _fields) {
    str_stream << HttpHeaderField::GetStringFromType(field_val.first) << ": "
               << field_val.second << "\r\n";
  }

  for(auto field_val : _unknown_fields) {
    str_stream << field_val.first << ": "
               << field_val.second << "\r\n";
  }
  str_stream << "\r\n";
  return str_stream.str();
}
