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


#include "Terminal.h"
#include "Data.h"
#include "Logger.h"

#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>


const static int READ_BUFF_SIZE = 1024;


Terminal::Terminal(uint32_t id, std::shared_ptr<TerminalListener> listener)
  : _id(id)
  , _master_fd(-1)
  , _child_pid(-1)
  , _listener(listener)
  , _read_enabled(true) {
}

Terminal::~Terminal() {
  if(_child_pid > 0) {
    int res_kill = kill(_child_pid, SIGKILL);
    if(res_kill != 0) {
      DLOG(error, "kill command failed : {}", res_kill);
    }
    int status = 0;
    int res_wait = waitpid(_child_pid, &status, 0);
    if(res_wait != _child_pid) {
      DLOG(info, "waitpid failed : {}, status : {}", res_wait, status);
    }
  }
}

uint32_t Terminal::GetId() {
  return _id;
}

bool Terminal::Init() {
  _child_pid= forkpty(&_master_fd, nullptr, nullptr, nullptr);
  if (_child_pid == -1) {
	  DLOG(error, "forkpty failed");
	  return false;
  }

  if (_child_pid != 0) {
    SetTrermAttributes();
	  fcntl(_master_fd, F_SETFL, O_NONBLOCK);
    Epool::GetInstance()->AddListener(shared_from_this(), true);
    return true;
  } else {
    setsid();
	  execlp("bash", "/bin/bash", NULL);
	  exit(0);
  }
  return true;
}

void Terminal::SetTrermAttributes() {
  //based on https://github.com/ovh/ovh-ttyrec/blob/master/ttyrec.c
	struct termios mastert;
  if((tcgetattr(_master_fd, &mastert) == 0) && (mastert.c_lflag == 0)) {
    mastert.c_iflag = IXON + ICRNL;  // 02400
    mastert.c_oflag = OPOST + ONLCR; // 05
    mastert.c_cflag = 0277;          //B38400 + CS8 + CREAD;
    mastert.c_lflag = ISIG + ICANON + ECHO + ECHOE + ECHOK + IEXTEN;
#ifdef ECHOKE
    mastert.c_lflag += ECHOKE;
#endif
#ifdef ECHOCTL
    mastert.c_lflag += ECHOCTL;
#endif
    // apply the c_cc config of a classic pseudotty given by posix_openpt()
    mastert.c_cc[VINTR] = 3;
    mastert.c_cc[VQUIT] = 28;
    mastert.c_cc[VERASE] = 127;
    mastert.c_cc[VKILL] = 21;
    mastert.c_cc[VEOF] = 4;
    mastert.c_cc[VTIME] = 0;
    mastert.c_cc[VMIN] = 1;
#ifdef VSWTC
    mastert.c_cc[VSWTC] = 0;
#endif
    mastert.c_cc[VSTART] = 17;
    mastert.c_cc[VSTOP] = 19;
    mastert.c_cc[VSUSP] = 26;
    mastert.c_cc[VEOL] = 0;
#ifdef VREPRINT
    mastert.c_cc[VREPRINT] = 18;
#endif
#ifdef VDISCARD
    mastert.c_cc[VDISCARD] = 15;
#endif
#ifdef VWERASE
    mastert.c_cc[VWERASE] = 23;
#endif
#ifdef VLNEXT
    mastert.c_cc[VLNEXT] = 22;
#endif
    tcsetattr(_master_fd, TCSANOW, &mastert);
  }
}

int Terminal::GetFd() {
  return _master_fd;
}

bool Terminal::Resize(int width, int height) {
  struct winsize win_size = {(unsigned short int)height, (unsigned short int)width, 0, 0};
  if(ioctl(_master_fd, TIOCSWINSZ, &win_size) == -1) {
    DLOG(error, "Resize failed using row/col: {}/{}", width, height);
    return false;
  }
  return true;
}

void Terminal::EnableRead(bool enable) {
  _read_enabled.store(enable);
  Epool::GetInstance()->SetListenerAwaitingRead(shared_from_this(), enable);
}

void Terminal::Write(const std::string& data) {
  ssize_t res = write(_master_fd, data.c_str(), data.length());
  if( res != (ssize_t)data.length()) {
    DLOG(error, "Write Failed");
  }
}

void Terminal::OnFdReadReady() {
  auto buff = std::make_shared<Data>(READ_BUFF_SIZE);
  ssize_t read_val = read(_master_fd, buff->GetCurrentDataRaw(), READ_BUFF_SIZE);
  if(read_val <= 0) {
    _listener->OnTerminalEnd(shared_from_this());
    return;
  }

  buff->SetCurrentSize((uint32_t)read_val);
  _listener->OnTerminalRead(shared_from_this(), buff);
  if(_read_enabled.load()) {
    Epool::GetInstance()->SetListenerAwaitingRead(shared_from_this(), true);
  }
}

void Terminal::OnFdWriteReady() {
}

void Terminal::OnFdOperationError(bool is_epool_err) {
}
