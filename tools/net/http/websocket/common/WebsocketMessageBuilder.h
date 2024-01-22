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

#include "Message.h"

#include <memory>
#include <vector>


class WebsocketMessage;
class WebsocketHeader;
class WebsocketFragmentBuilder;
class WebsocketDataCutter;

class WebsocketMessageBuilder : public MessageBuilder {
public:
  enum BuilderState {
    AWAITING_HEADER = 0,
    HEADER_PARSE_FAILED,
    RECEIVING_MESSAGE_BODY,
    MESSGAE_FRAGMENT_COMPLETED,
    MESSGAE_COMPLETED
  };
  WebsocketMessageBuilder();
  bool OnDataRead(std::shared_ptr<Data> data, std::vector<std::shared_ptr<Message> >& out_msgs) override;
  void SetState(BuilderState state);

private:
  void OnHeaderParseFailed();
  std::shared_ptr<WebsocketMessage> OnMessageData();
  std::shared_ptr<WebsocketMessage> OnMessageFragmentCompleted();
  std::shared_ptr<WebsocketMessage> OnMessageCompleted();

  std::unique_ptr<WebsocketDataCutter> _msg_cutter;
  std::unique_ptr<WebsocketFragmentBuilder> _fragment_builder;
  BuilderState _builder_state;
};
