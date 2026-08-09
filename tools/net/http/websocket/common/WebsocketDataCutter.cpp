/*
Copyright (c) 2023 - 2026 Adam Kaniewski

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

#include "WebsocketDataCutter.h"

#include "Data.h"
#include "DataResource.h"
#include "Logger.h"
#include "WebsocketFragmentBuilder.h"
#include "WebsocketHeader.h"
#include "WebsocketMessage.h"

WebsocketDataCutter::WebsocketDataCutter(WebsocketMessageBuilder& owner)
  : _owner(owner) {
}

bool WebsocketDataCutter::FindCutHeader(std::shared_ptr<Data> data, uint64_t& out_expected_cut_size) {
  out_expected_cut_size = 0;
  _header = WebsocketHeader::MaybeCreateFromRawData(data);
  if(!_header) {
    DLOG(error, "Failed to create websocket header");
    return false;
  }

  out_expected_cut_size = _header->_final_payload_len;

  _resource = std::make_shared<DataResource>();
  _resource->SetExpectedSize(_header->_final_payload_len);
  return true;
}

uint64_t WebsocketDataCutter::AddDataToCurrentCut(std::shared_ptr<Data> data) {
  _header->UnmaskData(data);
  _resource->AddData(data);
  return _resource->GetSize();
}

void WebsocketDataCutter::FindCutFooter(std::shared_ptr<Data> data) {
  std::shared_ptr<WebsocketMessage> msg;

  if(!_header->HasFinFlag()) {
    if(!_fragment_builder) {
      auto builder = new WebsocketFragmentBuilder(_header->_opcode, _resource);
      _fragment_builder = std::unique_ptr<WebsocketFragmentBuilder>(builder);
    } else {
      if(!_fragment_builder->AddFragment(_resource)) {
        //TODO
        DLOG(error, "Add fragment to builder failed");
        return;
      }
    }
  } else {
    if(_fragment_builder) {
      if(_header->HasControlOpCode()) {
        msg = std::make_shared<WebsocketMessage>(_header, _resource);
      } else {
        if(!_fragment_builder->AddFragment(_resource)) {
          //TODO
          DLOG(error, "Add fragment to builder failed");
          return;
        }
        _header->_opcode = _fragment_builder->GetOpcode();
        msg = std::make_shared<WebsocketMessage>(_header, _fragment_builder->GetResource());
        _fragment_builder = nullptr;
      }
    } else {
      msg = std::make_shared<WebsocketMessage>(_header, _resource);
    }
  }

  if(msg) {
    _messages_to_send.emplace_back(msg);
  }
}

std::vector<std::shared_ptr<WebsocketMessage>>& WebsocketDataCutter::GetMessagesToSend() {
  return _messages_to_send;
}