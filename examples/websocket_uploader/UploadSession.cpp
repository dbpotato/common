/*
Copyright (c) 2024 Adam Kaniewski

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

#include "UploadSession.h"
#include "Client.h"
#include "WebsocketMessage.h"
#include "Logger.h"
#include "JsonMsg.h"
#include "DataResource.h"
#include "Data.h"
#include "FileUtils.h"

#include <sstream>
#include <filesystem>


std::shared_ptr<UploadSession> UploadSession::Create(std::shared_ptr<Client> client, std::filesystem::path& save_dir) {
  std::shared_ptr<UploadSession> session;
  session.reset(new UploadSession(client,save_dir));
  if(session->Init()) {
    return session;
  }
  return nullptr;
}

UploadSession::UploadSession(std::shared_ptr<Client> client, std::filesystem::path& save_dir)
    : _state(State::WAIT_FOR_REQUEST)
    , _save_dir(save_dir)
    , _expected_chunk_size(0)
    , _expected_file_size(0)
    , _client(client) {
  _received_data = std::make_shared<DataResource>();
}

bool UploadSession::Init() {
  return CreateDirIfNotYetExist(_save_dir);
}

void UploadSession::ResetState() {
  _state = State::WAIT_FOR_REQUEST;
  _current_chunk = nullptr;
  _received_data = std::make_shared<DataResource>();
  _expected_chunk_size = 0;
  _expected_file_size = 0;
  _file_name = "";
  _file_directory_path = "";
}

void UploadSession::OnWsClientMessage(std::shared_ptr<WebsocketMessage> message) {
  if(_state == WAIT_FOR_REQUEST) {
    HandleRequest(message);
  } else if(_state == WAIT_FOR_CHUNK) {
    HandleDataChunk(message);
  } else {
    log()->error("Unexpected state : {}", (int)_state);
  }
}

void UploadSession::HandleRequest(std::shared_ptr<WebsocketMessage> message) {
  JsonMsg json;
  auto msg_resource = message->GetResource();
  if(msg_resource->UseDriveCache()) {
    std::string msg = "Unexpected large WebsocketMessage";
    log()->info(msg);
    _client->Send(std::make_shared<WebsocketMessage>(JsonMsg::MakeErrResponseMsg(5, msg)));
    ResetState();
    return;
  }

  std::string msg_str = msg_resource->GetMemCache()->ToString();
  if(json.Parse(msg_str)) {
    _file_name = json.ValueToString("file_name");
    _file_directory_path = json.ValueToString("file_directory_path");
    _expected_chunk_size = json.ValueToInt("expected_chunk_size");
    _expected_file_size = json.ValueToInt("expected_file_size");
  }

  log()->info("New upload req :\nname :{}, path: {}, chunk: {}, size: {}",
    _file_name,
    _file_directory_path,
    _expected_chunk_size,
    _expected_file_size);

  if(!_file_name.empty() &&
      _expected_chunk_size > 0 &&
      _expected_file_size > 0 &&
      CreateRequestedDir()) {
    _state = WAIT_FOR_CHUNK;
    auto msg = JsonMsg::MakeOkResponseMsg();
    _client->Send(std::make_shared<WebsocketMessage>(msg));
  } else {
    std::string err_msg = "Request rejected";
    auto msg = JsonMsg::MakeErrResponseMsg(4, err_msg);
    _client->Send(std::make_shared<WebsocketMessage>(msg));
    ResetState();
  }
}

bool UploadSession::CreateDirIfNotYetExist(std::filesystem::path& dir) {
  if(std::filesystem::is_directory(dir)) {
    return true;
  }
  std::error_code err;
  return std::filesystem::create_directories(dir, err);
}

bool UploadSession::CreateRequestedDir() {
  if(_file_directory_path.empty()) {
    return true;
  }
  auto target_dir = _save_dir / _file_directory_path;
  auto target_path = target_dir / _file_name;
  if(target_path.string().find("../") != std::string::npos) {
    return false;
  }
  return CreateDirIfNotYetExist(target_dir);
}

void UploadSession::HandleDataChunk(std::shared_ptr<WebsocketMessage> message) {
  _current_chunk = message->GetResource();
  if(_current_chunk->GetSize() > _expected_chunk_size) {
    log()->error("Chunk size is: {}, expected max: {}", _current_chunk->GetSize(), _expected_chunk_size);
    OnChunkSizeError();
    ResetState();
    return;
  }

  if(_current_chunk->GetSize() + _received_data->GetSize() > _expected_file_size) {
    log()->error("Current received data is: {}, expected : {}", _current_chunk->GetSize() + _received_data->GetSize(),
                 _expected_file_size);
    OnDataSizeError();
    ResetState();
    return;
  }

  if(_current_chunk->GetSize() + _received_data->GetSize() == _expected_file_size) {
    _received_data->AddData(_current_chunk);
    OnUploadCompleted();
    ResetState();
    return;
  }

  if(_current_chunk->GetSize() == _expected_chunk_size) {
    _received_data->AddData(_current_chunk);
    _current_chunk = std::make_shared<DataResource>();
    _client->Send(std::make_shared<WebsocketMessage>(JsonMsg::MakeOkResponseMsg()));
  }
}

void UploadSession::OnUploadCompleted() {
  log()->info("OnUploadCompleted");
  auto full_path = _save_dir / _file_directory_path / _file_name;
  if(_received_data->SaveToFile(full_path.string())) {
    _client->Send(std::make_shared<WebsocketMessage>(JsonMsg::MakeOkResponseMsg()));
  } else {
    std::string msg = "OnSaveError";
    log()->error("OnSaveError : {}", full_path.string());
    _client->Send(std::make_shared<WebsocketMessage>(JsonMsg::MakeErrResponseMsg(3, msg)));
  }
  ResetState();
}

void UploadSession::OnChunkSizeError() {
  std::string msg = "OnChunkSizeError";
  _client->Send(std::make_shared<WebsocketMessage>(JsonMsg::MakeErrResponseMsg(1, msg)));
}

void UploadSession::OnDataSizeError() {
  std::string msg = "OnDataSizeError";
  _client->Send(std::make_shared<WebsocketMessage>(JsonMsg::MakeErrResponseMsg(2, msg)));
}
