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

#pragma once

#include <memory>
#include <string>
#include "Epool.h"

class SysCall;

class SysCallMgr {
public :
  virtual void OnSysRead(std::shared_ptr<SysCall> syscall, const std::string& msg);
  virtual void OnSysFinished(std::shared_ptr<SysCall> syscall, bool success);
};

class SysCall : public std::enable_shared_from_this<SysCall>
              , public FdListener {
public:
  SysCall(std::weak_ptr<SysCallMgr> mgr);
  ~SysCall();
  void Run(const std::string commnad);
  bool Write(const std::string& msg);
  void AwaitFinish();

  int GetFd() override;
  void OnFdReadReady() override;
  void OnFdWriteReady() override;
  void OnFdOperationError(bool is_epool_err) override;

  const std::string& GetCommand() {return _cmd;}

private:
  enum State {
    STATE_CREATED = 0,
    STATE_RUNNING,
    STATE_FINISHED,
    STATE_KILLED
  };

  enum PipeFd {
    PARENT_READ = 0,
    PARENT_WRITE,
    CHILD_READ,
    CHILD_WRITE
  };

  void ClosePipe(PipeFd pfd);
  void ClosePipes();
  bool Create(const std::string& command);
  int GetPipe(PipeFd pfd);

  std::weak_ptr<SysCallMgr> _mgr;
  std::string _cmd;
  int _pipes[2][2];
  int _child_pid;
  State _state;
};