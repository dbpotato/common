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

#pragma once

#include "Data.h"
#include "Epool.h"

#include <atomic>
#include <memory>
#include <unistd.h>


class Terminal;

class TerminalListener {
public:
  virtual void OnTerminalRead(std::shared_ptr<Terminal> terminal, std::shared_ptr<Data> output) = 0;
  virtual void OnTerminalEnd(std::shared_ptr<Terminal> terminal) = 0;
};

class Terminal 
    : public FdListener
    , public std::enable_shared_from_this<Terminal> {
public :
  Terminal(uint32_t id, std::shared_ptr<TerminalListener> listener);
  ~Terminal();
  bool Init();
  uint32_t GetId();
  void Write(const std::string& data);
  bool Resize(int width, int height);
  void EnableRead(bool enable);

  //FdListener
  int GetFd() override;
  void OnFdReadReady() override;
  void OnFdWriteReady() override;

private :
  uint32_t _id;
  int _master_fd;
  int _child_pid;
  std::shared_ptr<TerminalListener> _listener;
  std::atomic_bool _read_enabled;
};
