/*
Copyright (c) 2019 Adam Kaniewski

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

#include <memory>

static const double CONNECT_TIMEOUT_IN_MS = 300.0;
static const int DEFAULT_SOCKET = -1;
static const int TRANSPORTER_SLEEP_IN_US = 100;
static const int TRANSPORTER_SELECT_TIMEOUT_IN_MS = 250;

enum NetError {
  OK = 0,
  RETRY,
  TIMEOUT,
  FAILED
};

class Data {
public:
  uint32_t _size;
  std::shared_ptr<unsigned char> _data;
  Data() : _size(0) {}
};

template<class T>
class LockablePtr {
private:
  std::weak_ptr<T> _weak;
  std::shared_ptr<T> _shared;
public:
  LockablePtr(std::weak_ptr<T> ptr) : _weak(ptr){}

  std::shared_ptr<T> Get() {
    return _shared;
  }

  std::shared_ptr<T> Lock() {
    _shared = _weak.lock();
    return _shared;
  }

  void Unlock() {
    _shared.reset();
  }
};
