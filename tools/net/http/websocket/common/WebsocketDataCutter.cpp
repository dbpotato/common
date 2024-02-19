/*
Copyright (c) 2023 - 2024 Adam Kaniewski

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
#include "WebsocketHeader.h"
#include "DataResource.h"
#include "Logger.h"


WebsocketDataCutter::WebsocketDataCutter(WebsocketMessageBuilder& owner)
  : _owner(owner) {
}

bool WebsocketDataCutter::FindCutHeader(std::shared_ptr<Data> data, uint32_t& out_expected_cut_size) {
  out_expected_cut_size = 0;
  _header = WebsocketHeader::MaybeCreateFromRawData(data);
  if(!_header) {
    DLOG(error, "Failed to create websocket header");
    _owner.SetState(WebsocketMessageBuilder::BuilderState::HEADER_PARSE_FAILED);
    return false;
  }

  out_expected_cut_size = _header->_final_payload_len;

  _owner.SetState(WebsocketMessageBuilder::BuilderState::RECEIVING_MESSAGE_BODY);
  _resource = std::make_shared<DataResource>();
  _resource->SetExpectedSize(_header->_final_payload_len);
  return true;
}

uint32_t WebsocketDataCutter::AddDataToCurrentCut(std::shared_ptr<Data> data) {
  if(_header->_mask) {
    auto buff = data->GetCurrentDataRaw();
    uint32_t size = data->GetCurrentSize();
    for(uint32_t i = 0; i < size; ++i) {
      unsigned char t = buff[i];
      t = t ^ _header->_mask_key[i % 4];
      buff[i] = t;
    }
  }
  _resource->AddData(data);
  return _resource->GetSize();
}

void WebsocketDataCutter::FindCutFooter(std::shared_ptr<Data> data) {
  if(_header->_fin) {
    _owner.SetState(WebsocketMessageBuilder::BuilderState::MESSGAE_COMPLETED);
  } else {
    _owner.SetState(WebsocketMessageBuilder::BuilderState::MESSGAE_FRAGMENT_COMPLETED);
  }
}

std::shared_ptr<DataResource> WebsocketDataCutter::GetResource() {
  return _resource;
}

std::shared_ptr<WebsocketHeader> WebsocketDataCutter::GetHeader() {
  return _header;
}