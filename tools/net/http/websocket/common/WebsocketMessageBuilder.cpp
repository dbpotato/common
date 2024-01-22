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

#include "WebsocketMessageBuilder.h"
#include "WebsocketDataCutter.h"
#include "WebsocketFragmentBuilder.h"
#include "WebsocketMessage.h"
#include "WebsocketHeader.h"
#include "Logger.h"


WebsocketMessageBuilder::WebsocketMessageBuilder() {
  _msg_cutter = std::unique_ptr<WebsocketDataCutter>(new WebsocketDataCutter(*this));
}

void WebsocketMessageBuilder::SetState(BuilderState state) {
  _builder_state = state;
}

bool WebsocketMessageBuilder::OnDataRead(std::shared_ptr<Data> data, std::vector<std::shared_ptr<Message> >& out_msgs) {
  if(!_msg_cutter->AddData(data)){
    return false;
  }

  std::shared_ptr<Message> msg;

  switch (_builder_state) {
    case BuilderState::HEADER_PARSE_FAILED :
      OnHeaderParseFailed();
      return false;
    case BuilderState::RECEIVING_MESSAGE_BODY :
      msg = OnMessageData();
      break;
    case BuilderState::MESSGAE_COMPLETED :
      msg = OnMessageCompleted();
      break;
    case BuilderState::MESSGAE_FRAGMENT_COMPLETED :
      msg = OnMessageFragmentCompleted();
      break;
    default:
      break;
  }

  if(msg) {
    out_msgs.push_back(msg);
  }
  return true;
}

void WebsocketMessageBuilder::OnHeaderParseFailed() {
  DLOG(error, "WebsocketMessageBuilder : Header Parse Failed");
}

std::shared_ptr<WebsocketMessage> WebsocketMessageBuilder::OnMessageData() {
  return std::make_shared<WebsocketMessage>(_msg_cutter->GetHeader(),
                                            _msg_cutter->GetResource());
}

std::shared_ptr<WebsocketMessage> WebsocketMessageBuilder::OnMessageFragmentCompleted() {
  if(!_fragment_builder) {
    auto builder = new WebsocketFragmentBuilder(_msg_cutter->GetHeader()->_opcode, _msg_cutter->GetResource());
    _fragment_builder = std::unique_ptr<WebsocketFragmentBuilder>(builder);
  } else {
    if(_fragment_builder->AddFragment(_msg_cutter->GetResource())) {
      DLOG(error, "WebsocketMessageBuilder : Add fragment to builder failed");
      return nullptr;
    }
  }
  return std::make_shared<WebsocketMessage>(_msg_cutter->GetHeader(),
                                            _fragment_builder->GetResource());
}

std::shared_ptr<WebsocketMessage> WebsocketMessageBuilder::OnMessageCompleted() {
  std::shared_ptr<WebsocketMessage> msg;
  auto header = _msg_cutter->GetHeader();

  if(_fragment_builder) {
    if(header->HasControlOpCode()){
      msg = std::make_shared<WebsocketMessage>(header, _msg_cutter->GetResource());
    } else {
      header->_opcode = _fragment_builder->GetOpcode();
      msg = std::make_shared<WebsocketMessage>(header, _fragment_builder->GetResource());
      _fragment_builder = nullptr;
    }
  } else {
    msg = std::make_shared<WebsocketMessage>(_msg_cutter->GetHeader(),
                                            _msg_cutter->GetResource());
  }
  return msg;
}
