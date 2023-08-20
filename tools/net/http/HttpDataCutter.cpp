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

#include "HttpMessage.h"
#include "DataResource.h"
#include "Logger.h"
#include "StringUtils.h"
#include "HttpDataCutter.h"
#include "HttpDataParser.h"

const int MAX_HEADER_LENGTH = 8*1024;
const int CHUNK_SEPARATOR_SIZE = 2; // "\r\n"
const int MAX_CHUNK_SIZE = 65535; // "FFFF\r\n"
const int MIN_CHUNK_HEADER_SIZE = 3; // "0\r\n"
const int HEADER_END_SIZE = 4; // "\r\n\r\n"


ChunkCutter::ChunkCutter(HttpMessageBuilder& owner, bool enable_drive_cache)
    : _owner(owner)
    , _enable_drive_cache(enable_drive_cache)
    , _pending_footer(false)
    , _end_found(false)
    , _joined_chunks(std::make_shared<DataResource>()) {
}

uint32_t ChunkCutter::AddDataToCurrentCut(std::shared_ptr<Data> data) {
  if(!_current_cut) {
    _current_cut = std::make_shared<DataResource>();
  }
  _current_cut->AddData(data);
  return _current_cut->GetSize();
}

bool ChunkCutter::FindCutHeader(std::shared_ptr<Data> data, uint32_t& out_expected_cut_size) {
  if(_pending_footer) {
    data->AddOffset(CHUNK_SEPARATOR_SIZE);
    _pending_footer = false;
  }

  bool result = HttpDataParser::FindChunkDataHeader(data, out_expected_cut_size);

  if(!result) {
    _owner.SetState(HttpMessageBuilder::BuilderState::HEADER_PARSE_FAILED);
    return result;
  }

  if(!out_expected_cut_size) {
    _owner.SetState(HttpMessageBuilder::BuilderState::CHUNK_MESSAGE_COMPLETED);
    _joined_chunks->SetExpectedSize(_joined_chunks->GetSize());
    _end_found = true;
  }

  return result;
}

void ChunkCutter::FindCutFooter(std::shared_ptr<Data> data) {
  if(_end_found) {
    _end_found = false;
    return;
  }

  bool result = HttpDataParser::FindChunkDataFooter(data);
  if(!result) {
    _pending_footer = true;
  }

  if(!_joined_chunks) {
    _joined_chunks = std::make_shared<DataResource>();
  }

  _joined_chunks->AddData(_current_cut);
  _current_cut = std::make_shared<DataResource>();
  _owner.SetState(HttpMessageBuilder::BuilderState::CHUNK_SEGMENT_COMPLETED);
}

std::shared_ptr<DataResource> ChunkCutter::GetResource() {
  return _joined_chunks;
}


//############


MsgCutter::MsgCutter(HttpMessageBuilder& owner, bool enable_drive_cache)
    : _owner(owner)
    , _enable_drive_cache(enable_drive_cache) {
}

uint32_t MsgCutter::AddDataToCurrentCut(std::shared_ptr<Data> data) {
  _resource->AddData(data);
  if(_resource->IsCompleted()) {
    UpdateBuilderState(HttpMessageBuilder::BuilderState::MESSGAE_COMPLETED);
  } else {
    UpdateBuilderState(HttpMessageBuilder::BuilderState::RECEIVING_MESSAGE_BODY);
  }
  return _resource->GetSize();
}

void MsgCutter::UpdateBuilderState(HttpMessageBuilder::BuilderState state) {
  bool ignore_notify = ((_last_state == HttpMessageBuilder::BuilderState::RECEIVING_CHUNKED ||
                        _last_state == HttpMessageBuilder::BuilderState::MESSGAE_COMPLETED) &&
                        state == HttpMessageBuilder::BuilderState::MESSGAE_COMPLETED);
  _last_state = state;
  if(!ignore_notify) {
    _owner.SetState(state);
  }
}

bool MsgCutter::FindCutHeader(std::shared_ptr<Data> data, uint32_t& out_expected_cut_size) {
  UpdateBuilderState(HttpMessageBuilder::BuilderState::AWAITING_HEADER);
  bool header_err = false;
  std::string transfer_encoding;

  _resource = std::make_shared<DataResource>(_enable_drive_cache);
  _header = HttpDataParser::FindContentDataHeader(data, out_expected_cut_size, header_err);

  if(!_header) {
    //not enough data;
    return true;
  } else if (header_err) {
    UpdateBuilderState(HttpMessageBuilder::BuilderState::HEADER_PARSE_FAILED);
    return false;
  }

  if(_header->HasField(HttpHeaderField::CONTENT_LENGTH)) {
    _resource->SetExpectedSize(out_expected_cut_size);
    return true;
  } else if(_header->GetFieldValue(HttpHeaderField::TRANSFER_ENCODING, transfer_encoding)) {
      if(!transfer_encoding.compare("chunked")) {
        UpdateBuilderState(HttpMessageBuilder::BuilderState::RECEIVING_CHUNKED);
        return true;
      } else {
        DLOG(error, "MsgCutter::FindCutHeader : unkonwn transfer encoding {}", transfer_encoding);
        return false;
      }
  }
  return true;
}

void MsgCutter::FindCutFooter(std::shared_ptr<Data> data) {
  UpdateBuilderState(HttpMessageBuilder::BuilderState::MESSGAE_COMPLETED);
}

std::shared_ptr<HttpHeader> MsgCutter::GetHeader() {
  return _header;
}

std::shared_ptr<DataResource> MsgCutter::GetResource() {
  return _resource;
}
