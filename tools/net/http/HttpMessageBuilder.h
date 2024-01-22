/*
Copyright (c) 2021 - 2023 Adam Kaniewski

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

#include "HttpHeader.h"
#include "Message.h"
#include "TapeCutter.h"

#include <vector>
#include <memory>

class DataResource;
class ResourceHolder;
class ChunkCutter;
class MsgCutter;
class HttpMessage;


class HttpMessageBuilder : public MessageBuilder{
public:
  enum BodyTransferMode {
    NONE = 0,
    CONTENT_LENGTH,
    CHUNKED
  };

  enum BuilderState {
    AWAITING_HEADER = 0,
    HEADER_PARSE_FAILED,
    RECEIVING_CHUNKED,
    RECEIVING_MESSAGE_BODY,
    MESSGAE_COMPLETED,
    CHUNK_SEGMENT_COMPLETED,
    CHUNK_MESSAGE_COMPLETED,
  };

  HttpMessageBuilder(bool enable_drive_cache = true);
  bool OnDataRead(std::shared_ptr<Data> data, std::vector<std::shared_ptr<Message> >& out_msgs) override;
  void SetState(BuilderState state);

private:
  void CreateMessage();
  BodyTransferMode _mode;
  std::shared_ptr<ChunkCutter> _chunk_cutter;
  std::shared_ptr<MsgCutter> _msg_cutter;
  BuilderState _builder_state;
  std::vector<std::shared_ptr<HttpMessage>> _messages_to_send;
};