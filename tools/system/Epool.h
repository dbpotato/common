/*
Copyright (c) 2020 - 2023 Adam Kaniewski

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

#include <map>
#include <memory>
#include <vector>

class SocketObject;
class ThreadLoop;


class FdListener {
public :
  virtual int GetFd() = 0;
  virtual void OnFdReadReady() = 0;
  virtual void OnFdWriteReady() = 0;
  virtual void OnFdOperationError(bool is_epool_err) = 0;
};

class FdListenerInfo {
public:
  FdListenerInfo(std::weak_ptr<FdListener> object);
  std::shared_ptr<FdListener> lock();
  std::weak_ptr<FdListener> _object;
  int _event_flags;
};


class Epool : public std::enable_shared_from_this<Epool> {
public:
  static std::shared_ptr<Epool> GetInstance();
  ~Epool();
  bool Init();
  void AddListener(std::shared_ptr<FdListener> obj, bool wait_for_read = false);
  void RemoveListener(int fd);
  void SetObservedEvent(int fd, int event_flag, bool enabled);

  void SetListenerAwaitingFlags(std::shared_ptr<FdListener> obj, bool waiting_for_read, bool waiting_for_write);
  void SetListenerAwaitingWrite(std::shared_ptr<FdListener> obj, bool waiting_for_write);
  void SetListenerAwaitingRead(std::shared_ptr<FdListener> obj, bool waiting_for_read);

protected:
  Epool();
  static std::weak_ptr<Epool> _instance;
private:
  bool CreateWakeFd();
  void Wake();
  void ClearWake();
  void WaitForEvents();
  void HandleFdEvent(int fd, int event);
  void NotifyListenerOnError(int fd, bool is_epool_err);
  void NotifyListenerOnError(std::shared_ptr<FdListener> obj, bool is_epool_err);
  int _epool_fd;
  int _wake_up_fd;
  std::map<int, FdListenerInfo> _listeners;
  std::shared_ptr<ThreadLoop> _thread_loop;
};
