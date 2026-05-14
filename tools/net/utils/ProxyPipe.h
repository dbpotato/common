/*
Copyright (c) 2026 Adam Kaniewski

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
#include <unistd.h>

#include "Client.h"

class Message;
class ThreadLoop;

class ProxyPipeListener {
public:
  virtual void OnProxyPipeEmpty();
  virtual void OnProxyPipePaused();
  virtual void OnProxyPipeResumed();
};


class ProxyPipe : public ClientManager,
                  public std::enable_shared_from_this<ProxyPipe> {
public:
  static std::shared_ptr<ProxyPipe> Create(
            std::shared_ptr<ThreadLoop> thread_loop,
            std::shared_ptr<Client> src,
            std::shared_ptr<Client> dest,
            uint32_t resume_read_low_val,
            uint32_t stop_read_high_val);
  static std::shared_ptr<ProxyPipe> Create(
            std::shared_ptr<ThreadLoop> thread_loop,
            std::shared_ptr<Client> src,
            std::shared_ptr<Client> dest);
  void SetListener(std::weak_ptr<ProxyPipeListener> listener);
  void PushMessage(std::shared_ptr<Message> msg);
  void OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) override;
  uint64_t GetTransferedDataSize();
protected:
  ProxyPipe(std::shared_ptr<ThreadLoop> thread_loop,
            std::shared_ptr<Client> src,
            std::shared_ptr<Client> dest,
            uint32_t resume_read_low_val,
            uint32_t stop_read_high_val);
private:
  std::shared_ptr<ThreadLoop> _thread_loop;
  std::shared_ptr<Client> _src;
  std::shared_ptr<Client> _dest;
  std::weak_ptr<ProxyPipeListener> _listener;
  uint32_t _active_msg_counter;
  uint32_t _stop_read;
  uint32_t _resume_read;
  uint64_t _transfered_data;
  bool _read_is_stopped;
};
