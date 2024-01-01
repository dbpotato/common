/*
Copyright (c) 2023 Adam Kaniewski

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

#include "DelayedTask.h"
#include "ThreadLoop.h"

#include <chrono>
#include <thread>


std::shared_ptr<DelayedTask> DelayedTask::Create(std::function<void()> post_function,
    std::weak_ptr<ThreadLoop> post_target,
    std::chrono::milliseconds delay,
    bool repeat) {
  std::shared_ptr<DelayedTask> task;
  task.reset(new DelayedTask(post_function,post_target, delay, repeat));
  task->Init();
  return task;
}

void DelayedTask::Cancel() {
  _run_thread.Stop();
  _self = nullptr;
}

DelayedTask::DelayedTask(std::function<void()> post_function,
                        std::weak_ptr<ThreadLoop> post_target,
                        std::chrono::milliseconds delay,
                        bool repeat)
    : _post_function(post_function)
    , _post_target(post_target)
    , _delay(delay)
    , _repeat(repeat) {
}

void DelayedTask::Init() {
  _self = shared_from_this();
  _run_thread.Run(_self, 0);
}

void DelayedTask::OnThreadStarted(int thread_id) {
  if(!_run_thread.ShouldRun() || !_self) {
    _self = nullptr;
    return;
  }

  do {
    std::this_thread::sleep_for(_delay);
    auto target_lock = _post_target.lock();
    if(target_lock && _self) {
      target_lock->Post(_post_function);
    }
  } while(_run_thread.ShouldRun() && _self && _repeat);
  _self = nullptr;
}
