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

#pragma once

#include "Data.h"
#include "TapeCutter.h"
#include "WebsocketMessageBuilder.h"


class WebsocketHeader;
class DataResource;

class WebsocketDataCutter : public TapeCutter {
public:
  WebsocketDataCutter(WebsocketMessageBuilder& owner);
  bool FindCutHeader(std::shared_ptr<Data> data, uint32_t& out_expected_cut_size) override;
  uint32_t AddDataToCurrentCut(std::shared_ptr<Data> data) override;
  void FindCutFooter(std::shared_ptr<Data> data) override;
  std::shared_ptr<DataResource> GetResource();
  std::shared_ptr<WebsocketHeader> GetHeader();
private:
  WebsocketMessageBuilder& _owner;
  std::shared_ptr<DataResource> _resource;
  std::shared_ptr<WebsocketHeader> _header;
};
