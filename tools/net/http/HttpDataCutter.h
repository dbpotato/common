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

#include "TapeCutter.h"
#include "HttpMessageBuilder.h"


class DataResource;

class ChunkCutter : public TapeCutter {
public:
  ChunkCutter(HttpMessageBuilder& owner, bool enable_drive_cache = true);
  uint64_t AddDataToCurrentCut(std::shared_ptr<Data> data) override;
  bool FindCutHeader(std::shared_ptr<Data> data, uint64_t& out_expected_cut_size) override;
  void FindCutFooter(std::shared_ptr<Data> data) override;
  std::shared_ptr<DataResource> GetResource();
private:
  HttpMessageBuilder& _owner;
  bool _enable_drive_cache;
  bool _pending_footer;
  bool _end_found;
  std::shared_ptr<DataResource> _current_cut;
  std::shared_ptr<DataResource> _joined_chunks;
};

class MsgCutter : public TapeCutter {
public:
  MsgCutter(HttpMessageBuilder& owner, bool enable_drive_cache = true);
  uint64_t AddDataToCurrentCut(std::shared_ptr<Data> data) override;
  bool FindCutHeader(std::shared_ptr<Data> data, uint64_t& out_expected_cut_size) override;
  void FindCutFooter(std::shared_ptr<Data> data) override;
  std::shared_ptr<HttpHeader> GetHeader();
  std::shared_ptr<DataResource> GetResource();
private:
  void UpdateBuilderState(HttpMessageBuilder::BuilderState state);
  HttpMessageBuilder& _owner;
  bool _enable_drive_cache;
  std::shared_ptr<HttpHeader> _header;
  std::shared_ptr<DataResource> _resource;
  HttpMessageBuilder::BuilderState _last_state;
};
