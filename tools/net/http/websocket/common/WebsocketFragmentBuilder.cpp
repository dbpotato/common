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

#include "WebsocketFragmentBuilder.h"
#include "WebsocketMessage.h"
#include "WebsocketHeader.h"
#include "Logger.h"

#include <cstring>

WebsocketFragmentBuilder::WebsocketFragmentBuilder()
    : _original_opcode(0)
    , _collected_size(0) {
}

bool WebsocketFragmentBuilder::AddFragment(std::shared_ptr<WebsocketMessage> fragment) {
  std::shared_ptr<WebsocketHeader> header = fragment->_header;
  if(!_collected_fragments.size()) {
    if(header->_fin || !header->_opcode) {
      log()->error("WebsocketFragmentBuilder : Invalid initial frame");
      return false;
    } else {
      _original_opcode = header->_opcode;
    }
  } else {
    if((header->_fin && fragment->_header->_opcode) ||
        (!fragment->_header->_fin && fragment->_header->_opcode) ) {
      return false;
    }
  }

  _collected_size += fragment->_size;
  _collected_fragments.push_back(Data(fragment->_size, fragment->_data));

  return true;
}

std::shared_ptr<WebsocketMessage> WebsocketFragmentBuilder::MergeFragments() {
  if(!_collected_fragments.size()) {
    return {};
  }

  std::shared_ptr<WebsocketHeader> new_header = std::make_shared<WebsocketHeader>();
  new_header->_fin = 1;
  new_header->_opcode = _original_opcode;
  new_header->_final_payload_len = _collected_size;

  std::shared_ptr<unsigned char> payload_data;

  if(_collected_size) {
    payload_data = std::shared_ptr<unsigned char>(new unsigned char[_collected_size],
                                                       std::default_delete<unsigned char[]>());
    uint32_t offset = 0;
    for(Data& data : _collected_fragments) {
      std::memcpy(payload_data.get() + offset, data._data.get(), data._size);
      offset += data._size;
    }
  }

  std::shared_ptr<WebsocketMessage> result = std::make_shared<WebsocketMessage>(_collected_size, payload_data);
  result->_header = new_header;
  return result;
}
