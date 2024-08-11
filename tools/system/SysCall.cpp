/*
Copyright (c) 2022 - 2024 Adam Kaniewski

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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <paths.h>
#include <cstring>
#include <unistd.h>

#include "SysCall.h"
#include "Logger.h"
#include "Data.h"

extern char **environ;
const size_t READ_BUFF_SIZE = 256;
int INVALID_FD = 0;

SysLauncher::SysLauncher() : _child_pid(-1) {
  _pipes[0][0] = _pipes[0][1] = _pipes[1][0] = _pipes[1][1] = 0; 
}

SysLauncher::~SysLauncher() {
  if(_child_pid > 0) {
    ClosePipes();
    kill(_child_pid, SIGKILL);
  }
}

int SysLauncher::GetPipe(PipeFd pfd) {
  switch(pfd) {
    case PARENT_READ:
      return _pipes[0][0];
    case PARENT_WRITE:
      return _pipes[1][1];
    case  CHILD_READ :
      return _pipes[1][0];
    case CHILD_WRITE:
      return _pipes[0][1];
    default:
      return INVALID_FD;
  }
  return INVALID_FD;
}

void SysLauncher::ClosePipe(PipeFd pfd) {
  int pipe = GetPipe(pfd);
  if(pipe) {
    close(pipe);
    pipe = 0;
  }
}

void SysLauncher::ClosePipes() {
  ClosePipe(PARENT_READ);
  ClosePipe(PARENT_WRITE);
  ClosePipe(CHILD_READ);
  ClosePipe(CHILD_WRITE);
}

bool SysLauncher::Create(const std::string& command) {
  if(pipe(_pipes[0]) !=0)
    return false;
  if(pipe(_pipes[1]) !=0)
    return false;

  _child_pid = fork();
  if(_child_pid < 0 )
    return false;

  if(!_child_pid) {
    char *argp[] = {strdup("sh"),
                    strdup("-c"),
                    strdup(command.c_str()),
                    nullptr};

    dup2(GetPipe(CHILD_READ), STDIN_FILENO);
    dup2(GetPipe(CHILD_WRITE), STDOUT_FILENO);

    ClosePipes();

    execve(_PATH_BSHELL, argp, environ);

    for(int i = 0; i<3; i++) {
      free(argp[i]);
    }

    _exit(127);
  } else {
    ClosePipe(CHILD_READ);
    ClosePipe(CHILD_WRITE);
    fcntl(GetPipe(PARENT_READ), F_SETFL, O_NONBLOCK);
  }
  return true;
}

void SysCallMgr::OnSysRead(std::shared_ptr<SysCall> syscall, const std::string& msg) {
}

void SysCallMgr::OnSysFinished(std::shared_ptr<SysCall> syscall, bool success) {
}

SysCall::SysCall(std::shared_ptr<SysCallMgr> mgr)
    : _mgr(mgr){
}

int SysCall::GetFd() {
  return GetPipe(PARENT_READ);
}

bool SysCall::Write(const std::string& msg) {
  size_t res = write(GetPipe(PARENT_WRITE),
                    msg.c_str(),
                    msg.length());
  if(res != msg.length()) {
    return false;
  }
  return true;
}

void SysCall::OnFdReadReady() {
  auto buff = std::make_shared<Data>(READ_BUFF_SIZE);
  ssize_t read_size = read(GetPipe(PARENT_READ), buff->GetCurrentDataRaw(), READ_BUFF_SIZE);

  if(read_size > 0) {
    buff->SetCurrentSize(read_size);
    _mgr->OnSysRead(shared_from_this(), buff->ToString());
    Epool::GetInstance()->SetListenerAwaitingRead(shared_from_this(), true);
  } else {
    _mgr->OnSysFinished(shared_from_this(), (read_size == 0));
  }
}

void SysCall::OnFdWriteReady() {

}

void SysCall::OnFdOperationError(bool is_epool_err) {
  _mgr->OnSysFinished(shared_from_this(), false);
}

void SysCall::Run(const std::string commnad) {
  _cmd = commnad;
  if(!Create(_cmd)) {
    _mgr->OnSysFinished(shared_from_this(), false);
    return;
  }
  Epool::GetInstance()->AddListener(shared_from_this(), true);
}