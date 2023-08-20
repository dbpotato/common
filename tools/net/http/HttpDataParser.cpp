/*
Copyright (c) 2023 Adam Kaniewski

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

#include "HttpDataParser.h"
#include "HttpHeader.h"
#include "StringUtils.h"
#include "Logger.h"

const int MAX_HEADER_LENGTH = 8*1024;
const int CHUNK_SEPARATOR_SIZE = 2; // "\r\n"
const int MAX_CHUNK_SIZE = 65535; // "FFFF\r\n"
const int MIN_CHUNK_HEADER_SIZE = 3; // "0\r\n"
const int HEADER_END_SIZE = 4; // "\r\n\r\n"


std::shared_ptr<HttpHeader> HttpDataParser::FindContentDataHeader(std::shared_ptr<Data> data, uint32_t& out_expected_cut_size, bool& header_err) {
  out_expected_cut_size = 0;
  header_err = false;
  std::shared_ptr<HttpHeader> header;

  auto data_str = std::string((const char*)data->GetCurrentDataRaw(), (int)data->GetCurrentSize());
  auto header_end = data_str.find("\r\n\r\n");
  if( header_end == std::string::npos) {
    return nullptr;
  }

  std::string header_str = std::string((const char*)data->GetCurrentDataRaw(), header_end);
  header = HttpHeader::Parse(header_str);
  data->AddOffset(header_end + HEADER_END_SIZE);

  if(header) {
    std::string content_len_val_str;
    std::string transfer_encoding;
    if(header->GetFieldValue(HttpHeaderField::CONTENT_LENGTH, content_len_val_str)) {
      long content_len = 0;
      if(!StringUtils::ToInt(content_len_val_str, content_len)) {
        DLOG(error, "Cant convert to int http body size : {}", content_len_val_str);
        header_err = true;
      }
      out_expected_cut_size = (uint32_t) content_len;
    }
  }
  return header;
}

bool HttpDataParser::FindChunkDataHeader(std::shared_ptr<Data> data, uint32_t& out_expected_cut_size) {
  out_expected_cut_size = 0;

  if(data->GetCurrentSize() < MIN_CHUNK_HEADER_SIZE) {
    return false;
  }

  std::string header_str = std::string((const char*)data->GetCurrentDataRaw(),
                                       (int)data->GetCurrentSize());
  std::vector<std::string> header_split = StringUtils::Split(header_str, "\r\n");
  if(!header_split.size()) {
    return false;
  }

  std::string hex_lenght_str = header_split.at(0);
  int chunk_lenght = 0;
  if(!StringUtils::ToInt(hex_lenght_str, chunk_lenght, 16)) {
    DLOG(error, "Cant parse http chunk size : {}", hex_lenght_str.length());
    return false;
  }

  if(chunk_lenght > MAX_CHUNK_SIZE ) {
    DLOG(error, "Http chunk size exceed max size");
    return false;
  }
  out_expected_cut_size = chunk_lenght;
  data->AddOffset(hex_lenght_str.length() + CHUNK_SEPARATOR_SIZE);
  return true;
}

bool HttpDataParser::FindChunkDataFooter(std::shared_ptr<Data> data) {
  if(data->GetCurrentSize() >= CHUNK_SEPARATOR_SIZE) {
    data->AddOffset(CHUNK_SEPARATOR_SIZE);
    return true;
  } else {
    return false;
  }
}
