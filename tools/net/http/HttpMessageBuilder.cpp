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


#include "HttpMessageBuilder.h"
#include "HttpMessage.h"
#include "DataResource.h"
#include "Logger.h"
#include "StringUtils.h"
#include "TapeCutter.h"
#include "HttpDataCutter.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>


HttpMessageBuilder::HttpMessageBuilder(bool enable_drive_cache)
    : MessageBuilder() {
  _msg_cutter = std::make_shared<MsgCutter>(*this, enable_drive_cache);
  _chunk_cutter = std::make_shared<ChunkCutter>(*this, enable_drive_cache);
}

bool HttpMessageBuilder::AddData(std::shared_ptr<Data> data, std::vector<std::shared_ptr<Message> >& out_msgs) {
  bool data_add_success = false;

  if(_mode == BodyTransferMode::CHUNKED) {
    data_add_success = _chunk_cutter->AddData(data);
  } else {
    data_add_success =_msg_cutter->AddData(data);
    if(_mode == BodyTransferMode::CHUNKED && data_add_success) {
      data_add_success = _chunk_cutter->AddData(data);
    }
  }

  if(!data_add_success) {
    DLOG(error, "HttpMessageBuilder::AddData Failed");
    return false;
  }

  out_msgs.insert(out_msgs.end(), _messages_to_send.begin(), _messages_to_send.end());
  _messages_to_send.clear();
  return true;
}

void HttpMessageBuilder::CreateMessage() {
  std::shared_ptr<DataResource> resource;
  if(_mode == BodyTransferMode::CHUNKED) {
    resource = _chunk_cutter->GetResource();
  } else {
    resource = _msg_cutter->GetResource();
  }

  for(auto msg : _messages_to_send) {
    if(msg->GetResource() == resource) {
      return;
    }
  }

  _messages_to_send.push_back(std::make_shared<HttpMessage>(_msg_cutter->GetHeader(), resource));
}

void HttpMessageBuilder::SetState(BuilderState state) {
  _builder_state = state;
  switch (_builder_state) {
    case BuilderState::AWAITING_HEADER :
      _mode = BodyTransferMode::NONE;
      break;
    case BuilderState::RECEIVING_CHUNKED :
      _mode = BodyTransferMode::CHUNKED;
      break;
    case BuilderState::RECEIVING_MESSAGE_BODY :
      _mode = BodyTransferMode::CONTENT_LENGTH;
      CreateMessage();
      break;
    case BuilderState::MESSGAE_COMPLETED :
    case BuilderState::CHUNK_SEGMENT_COMPLETED :
    case BuilderState::CHUNK_MESSAGE_COMPLETED :
      CreateMessage();
      break;
    default:
      break;
  }
}
