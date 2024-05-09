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

#pragma once

#include <memory>
#include <string>
#include <filesystem>

class Client;
class DataResource;
class WebsocketMessage;

class UploadSession {
public:
  enum State {
    WAIT_FOR_REQUEST = 0,
    WAIT_FOR_CHUNK
  };
  static std::shared_ptr<UploadSession> Create(std::shared_ptr<Client> client, std::filesystem::path& save_dir);
  void OnWsClientMessage(std::shared_ptr<WebsocketMessage> message);
protected:
  UploadSession(std::shared_ptr<Client> client, std::filesystem::path& save_dir);
  bool Init();
private :
  void HandleRequest(std::shared_ptr<WebsocketMessage> message);
  void HandleDataChunk(std::shared_ptr<WebsocketMessage> message);
  void OnUploadCompleted();
  void OnChunkSizeError();
  void OnDataSizeError();
  void ResetState();
  bool CreateRequestedDir();
  bool CreateDirIfNotYetExist(std::filesystem::path& dir);

  State _state;
  std::filesystem::path _save_dir;
  std::string _file_name;
  std::string _file_directory_path;
  size_t _expected_chunk_size;
  size_t _expected_file_size;
  std::shared_ptr<Client> _client;
  std::shared_ptr<DataResource> _current_chunk;
  std::shared_ptr<DataResource> _received_data;
};
