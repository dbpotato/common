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

#include "HttpHeaderDecl.h"

#include <memory>
#include <map>
#include <string>


class HttpHeader {
public :
  HttpHeader(HttpHeaderProtocol::Type protocol, int status_code);
  HttpHeader(HttpHeaderProtocol::Type protocol, HttpHeaderMethod::Type method, const std::string& request);
  static std::shared_ptr<HttpHeader> Parse(const std::string& header_str);
  void AddField(HttpHeaderField::Type, const std::string& value);
  bool HasField(HttpHeaderField::Type type);
  bool GetFieldValue(HttpHeaderField::Type type, std::string& out_value);
  bool IsValid();
  std::string ToString();

  HttpHeaderProtocol::Type GetProtocol(){return _protocol;}
  HttpHeaderMethod::Type GetMethod(){return _method;}
  int GetStatusCode() {return _status_code;}
  const std::string& GetRequestTarget() {return _request_target;}

protected :
  HttpHeaderProtocol::Type _protocol;
  HttpHeaderMethod::Type _method;
  int _status_code;
  std::string _request_target;
  std::map<HttpHeaderField::Type, std::string> _fields;
  HttpHeader();
  bool ParseType(const std::string& header_first_line);
  bool ParseStatusCode(const std::string& code_str);
  bool KeyValSplit(const std::string& header_line, std::string& key, std::string& val);
};
