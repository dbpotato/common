/*
Copyright (c) 2022 - 2026 Adam Kaniewski

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


WebsocketMessageBuilder::WebsocketMessageBuilder() {
  _msg_cutter = std::unique_ptr<WebsocketDataCutter>(new WebsocketDataCutter(*this));
}

bool WebsocketMessageBuilder::OnDataRead(std::shared_ptr<Data> data, std::vector<std::shared_ptr<Message> >& out_msgs) {
  if(!_msg_cutter->AddData(data)){
    return false;
  }

  auto& messages_to_send = _msg_cutter->GetMessagesToSend();
  out_msgs.insert(out_msgs.end(), messages_to_send.begin(), messages_to_send.end());
  messages_to_send.clear();

  return true;
}
