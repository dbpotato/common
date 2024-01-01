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

#pragma once

#include <chrono>
#include <functional>
#include <memory>


#include "PosixThread.h"

class ThreadLoop;

class DelayedTask: public std::enable_shared_from_this<DelayedTask>
                 , public ThreadObject {

friend class PosixThread;

public:
static std::shared_ptr<DelayedTask> Create(std::function<void()> post_function,
        std::weak_ptr<ThreadLoop> post_target,
        std::chrono::milliseconds delay,
        bool repeat = false);
void Cancel();
protected:
  DelayedTask(std::function<void()> post_function,
              std::weak_ptr<ThreadLoop> post_target,
              std::chrono::milliseconds delay,
              bool repeat);
  void Init();
  void OnThreadStarted(int thread_id) override;

private:
  std::function<void()> _post_function;
  std::weak_ptr<ThreadLoop> _post_target;
  std::chrono::milliseconds _delay;
  bool _repeat;
  PosixThread _run_thread;
  std::shared_ptr<DelayedTask> _self;
};
