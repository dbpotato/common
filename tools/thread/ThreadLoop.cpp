/*
Copyright (c) 2020 - 2021 Adam Kaniewski

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

#include "ThreadLoop.h"

void ThreadLoop::Init() {
  _run_thread.Run(shared_from_this(), 0);
}

void ThreadLoop::Post(std::function<void()> request) {
  std::unique_lock<std::mutex> lock(_condition_mutex);
  _msgs.push(request);
  _condition.notify_one();
}

bool ThreadLoop::OnDifferentThread() {
  return !_run_thread.OnSameThread();
}

void ThreadLoop::OnThreadStarted(int thread_id) {
  while(_run_thread.ShouldRun()) {

    std::function<void()> msg;
    {
      std::unique_lock<std::mutex> lock(_condition_mutex);
      _condition.wait(lock, [this]{return !_msgs.empty() || !_run_thread.ShouldRun();});
      msg = _msgs.front();
      _msgs.pop();
    }

    if(msg)
      msg();
    else
      return;
  }
}
