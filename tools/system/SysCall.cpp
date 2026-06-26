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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <paths.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>


#include "SysCall.h"
#include "Logger.h"
#include "Data.h"
#include "AsyncTask.h"

extern char **environ;
const size_t READ_BUFF_SIZE = 256;
static int INVALID_FD = -1;



void SysCallMgr::OnSysRead(std::shared_ptr<SysCall> syscall, const std::string& msg) {
}

void SysCallMgr::OnSysFinished(std::shared_ptr<SysCall> syscall, bool success) {
}


SysCall::SysCall(std::weak_ptr<SysCallMgr> mgr)
    : _mgr(mgr)
    , _child_pid(-1) {
  _pipes[0][0] = _pipes[0][1] = _pipes[1][0] = _pipes[1][1] = INVALID_FD;
}

SysCall::~SysCall() {
  if(_child_pid > 0 && _state == STATE_RUNNING) {
    _state = STATE_KILLED;
    kill(_child_pid, SIGKILL);
    waitpid(_child_pid, nullptr, WNOHANG);
  }
  ClosePipes();
}

int SysCall::GetPipe(PipeFd pfd) {
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

void SysCall::ClosePipe(PipeFd pfd) {
  int pipe = GetPipe(pfd);
  if(pipe != INVALID_FD) {
    close(pipe);
    switch(pfd) {
      case PARENT_READ:     _pipes[0][0] = INVALID_FD; break;
      case PARENT_WRITE:    _pipes[1][1] = INVALID_FD; break;
      case CHILD_READ:      _pipes[1][0] = INVALID_FD; break;
      case CHILD_WRITE:     _pipes[0][1] = INVALID_FD; break;
    }
  }
}

void SysCall::ClosePipes() {
  ClosePipe(PARENT_READ);
  ClosePipe(PARENT_WRITE);
  ClosePipe(CHILD_READ);
  ClosePipe(CHILD_WRITE);
}

bool SysCall::Create(const std::string& command) {
  if(pipe(_pipes[0]) !=0 || pipe(_pipes[1]) != 0) {
    DLOG(error, "SysCall pipe init failed");
    ClosePipes();
    return false;
  }

  _child_pid = fork();
  if(_child_pid < 0 ) {
    DLOG(error, "SysCall fork failed");
    ClosePipes();
    return false;
  }

  if(!_child_pid) {
    char *command_dup = strdup(command.c_str());
    char *param_dup = strdup("-c");
    char *argp[] = {_PATH_BSHELL, param_dup, command_dup, nullptr};

    dup2(GetPipe(CHILD_READ), STDIN_FILENO);
    dup2(GetPipe(CHILD_WRITE), STDOUT_FILENO);

    ClosePipes();

    execve(_PATH_BSHELL, argp, environ);

    free(command_dup);
    free(param_dup);

    DLOG(error, "SysCall execve failed for: %s", command.c_str());
    _exit(127);
  } else {
    _state = STATE_RUNNING;
    AsyncTask::Create(std::bind(&SysCall::AwaitFinish, shared_from_this()));
    ClosePipe(CHILD_READ);
    ClosePipe(CHILD_WRITE);
    fcntl(GetPipe(PARENT_READ), F_SETFL, O_NONBLOCK);
  }
  return true;
}

int SysCall::GetFd() {
  return GetPipe(PARENT_READ);
}

bool SysCall::Write(const std::string& msg) {
  auto mgr = _mgr.lock();
  if(_state != STATE_RUNNING || !mgr) {
    return false;
  }

  sigset_t mask, old_mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &mask, &old_mask);

  size_t total_written = 0;
  while(total_written < msg.length()) {
    ssize_t res = write(GetPipe(PARENT_WRITE),
                        msg.c_str() + total_written,
                        msg.length() - total_written);
    if(res < 0) {
      if(errno == EINTR) continue;
      if(errno == EPIPE || errno == EIO) {
        pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
        _state = STATE_FINISHED;
        mgr->OnSysFinished(shared_from_this(), false);
        return false;
      }
      if(errno == EAGAIN) {
        pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
        return false;
      }
    } else if(res > 0) {
      total_written += static_cast<size_t>(res);
    } else {
      break;
    }
  }

  pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
  return (total_written == msg.length());
}

void SysCall::OnFdReadReady() {
  auto mgr = _mgr.lock();
  if(_state != STATE_RUNNING || !mgr) {
    return;
  }

  auto buff = std::make_shared<Data>(READ_BUFF_SIZE);
  ssize_t read_size = read(GetPipe(PARENT_READ), buff->GetCurrentDataRaw(), READ_BUFF_SIZE);

  if(read_size > 0) {
    buff->SetCurrentSize(read_size);
    mgr->OnSysRead(shared_from_this(), buff->ToString());
    Epool::GetInstance()->SetListenerAwaitingRead(shared_from_this(), true);
  } else if(read_size == 0) {
    _state = STATE_FINISHED;
  } else if(errno != EAGAIN && errno != EINTR) {
    DLOG(error, "SysCall read error: %s", strerror(errno));
    _state = STATE_FINISHED;
    mgr->OnSysFinished(shared_from_this(), false);
  }
}

void SysCall::OnFdWriteReady() {
}

void SysCall::OnFdOperationError(bool is_epool_err) {
  auto mgr = _mgr.lock();
  if(mgr) {
    mgr->OnSysFinished(shared_from_this(), false);
  }
}

void SysCall::Run(const std::string commnad) {
  _cmd = commnad;
  if(!Create(_cmd)) {
    auto mgr = _mgr.lock();
    if(mgr) {
      mgr->OnSysFinished(shared_from_this(), false);
    }
    return;
  }
  Epool::GetInstance()->AddListener(shared_from_this(), true);
}

void SysCall::AwaitFinish() {
  int status = 0;
  pid_t result;
  do {
    result = waitpid(_child_pid, &status, 0);
  } while(result < 0 && errno == EINTR);

  auto mgr = _mgr.lock();

  if(result < 0) {
    if(errno == ECHILD) {
      return;
    }
    DLOG(error, "SysCall waitpid failed: %s", strerror(errno));
    if(mgr) {
      mgr->OnSysFinished(shared_from_this(), false);
    }
    return;
  }

  if(_state == STATE_RUNNING && mgr) {
    bool success = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    _state = STATE_FINISHED;
    mgr->OnSysFinished(shared_from_this(), success);
  }
}
