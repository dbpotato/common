/*
Copyright (c) 2018 Adam Kaniewski

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

#include "PosixThread.h"
#include "Logger.h"

#include <cstdio>


PosixThread::PosixThread()
    : _id(0)
    , _is_running(false)
    , _should_run(false) {
}

void* PosixThread::StartThread(void *obj) {
  PosixThread* thread = (PosixThread*) obj;
  if(auto obj = thread->GetObj().lock()) {
    obj->OnThreadStarted(thread->_id);
  }
  thread->_is_running = false;
  thread->_should_run = false;
  return nullptr;
}

bool PosixThread::Run(std::weak_ptr<ThreadObject> obj, int id) {
  if(!_is_running) {
    _id = id;
    _is_running = true;
    _should_run = true;
    _thread_obj = obj;
    if(int res = pthread_create(&_thread, NULL, PosixThread::StartThread, this) != 0) {
      DLOG(error,"Failed to create thread : {}, code : {}", id, res);
      return false;
    }
  }
  else {
    DLOG(error,"Thread already runs: {}", id);
    return false;
  }
  return true;
}

std::weak_ptr<ThreadObject> PosixThread::GetObj() {
  return _thread_obj;
}

void PosixThread::Stop() {
  _should_run = false;
}

std::atomic_bool& PosixThread::IsRunning() {
  return _is_running;
}

std::atomic_bool& PosixThread::ShouldRun() {
  return _should_run;
}

void PosixThread::Join() {
  if(_is_running)
    pthread_join(_thread, NULL);
}
