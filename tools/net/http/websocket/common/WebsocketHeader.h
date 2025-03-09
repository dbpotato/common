/*
Copyright (c) 2022 - 2023 Adam Kaniewski

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

#include <cstdint>
#include <memory>

#include "Data.h"


class WebsocketHeader {
public :
  enum OpCode {
    CONTINUE = 0,
    TEXT = 1,
    BINARY = 2,
    CLOSE = 8,
    PING = 9,
    PONG = 10
  };

  WebsocketHeader();
  WebsocketHeader(OpCode code, uint64_t data_length = 0);

  static std::shared_ptr<WebsocketHeader> MaybeCreateFromRawData(std::shared_ptr<Data> data);
  void ParseFirstBytes(uint8_t header_start, uint8_t mask_with_payload);
  bool FindPayloadAndHeaderSize(std::shared_ptr<Data> data);
  void FindMaskKey(std::shared_ptr<Data> data);
  bool HasControlOpCode();
  bool HasFinFlag();
  void CreateBinaryForm();
  std::shared_ptr<Data> GetBinaryForm();


  uint8_t _fin : 1;
  uint8_t _rsv1 : 1;
  uint8_t _rsv2 : 1;
  uint8_t _rsv3 : 1;
  uint8_t _opcode : 4;
  uint8_t _mask : 1;
  uint8_t _payload_len : 7;
  uint64_t _final_payload_len;
  uint8_t _mask_key[4];

  uint32_t _header_length;
  std::shared_ptr<Data> _binary_form;
};
