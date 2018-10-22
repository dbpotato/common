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

#pragma once

#include <pthread.h>
#include <atomic>
#include <memory>


class ThreadObject {
 public :
  virtual void OnThreadStarted(int thread_id) = 0;
};


class PosixThread {
protected:
  std::weak_ptr<ThreadObject> _thread_obj;
  pthread_t _thread;
  int _id;
  std::atomic_bool _is_running;
  std::atomic_bool _should_run;
  static void* StartThread(void* obj);

public:
  PosixThread();
  bool Run(std::weak_ptr<ThreadObject> obj, int id = 0);
  std::weak_ptr<ThreadObject> GetObj();
  void Stop();
  std::atomic_bool& IsRunning();
  std::atomic_bool& ShouldRun();
  void Join();
};
