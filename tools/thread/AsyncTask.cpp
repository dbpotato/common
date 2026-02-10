/*
Copyright (c) 2025 Adam Kaniewski

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

#include "AsyncTask.h"

#include <thread>


std::shared_ptr<AsyncTask> AsyncTask::Create(std::function<void()> post_function) {
  std::shared_ptr<AsyncTask> task;
  task.reset(new AsyncTask(post_function));
  task->Init();
  return task;
}

void AsyncTask::Cancel() {
  _run_thread.Stop();
  _self = nullptr;
}

AsyncTask::AsyncTask(std::function<void()> post_function)
    : _post_function(post_function) {
}

void AsyncTask::Init() {
  _self = shared_from_this();
  _run_thread.Run(_self, 0);
}

void AsyncTask::OnThreadStarted(int thread_id) {
  if(!_run_thread.ShouldRun() || !_self) {
    _self = nullptr;
    return;
  }
  _post_function();
  _self = nullptr;
}
