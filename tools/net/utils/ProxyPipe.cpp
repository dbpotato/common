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

#include "ProxyPipe.h"
#include "Logger.h"
#include "ThreadLoop.h"


const uint32_t DEFAULT_STOP_READ = 32;
const uint32_t DEFAULT_RESUME_READ = 8;


void ProxyPipeListener::OnProxyPipeEmpty(){}
void ProxyPipeListener::OnProxyPipePaused(){}
void ProxyPipeListener::OnProxyPipeResumed(){}


std::shared_ptr<ProxyPipe> ProxyPipe::Create(
            std::shared_ptr<ThreadLoop> thread_loop,
            std::shared_ptr<Client> src,
            std::shared_ptr<Client> dest,
            uint32_t resume_read_low_val,
            uint32_t stop_read_high_val) {
  std::shared_ptr<ProxyPipe> result;
  if(src && dest){
    result.reset(new ProxyPipe(thread_loop, src, dest, resume_read_low_val, stop_read_high_val));
    dest->AddListener(result);
  }
  return result;
}

std::shared_ptr<ProxyPipe> ProxyPipe::Create(
            std::shared_ptr<ThreadLoop> thread_loop,
            std::shared_ptr<Client> src,
            std::shared_ptr<Client> dest) {
  return Create(thread_loop, src, dest, DEFAULT_STOP_READ, DEFAULT_RESUME_READ);
}

ProxyPipe::ProxyPipe(std::shared_ptr<ThreadLoop> thread_loop,
        std::shared_ptr<Client> src,
        std::shared_ptr<Client> dest,
        uint32_t stop_read,
        uint32_t resume_read)
    : _thread_loop(thread_loop)
    , _src(src)
    , _dest(dest)
    , _active_msg_counter(0)
    , _stop_read(stop_read)
    , _resume_read(resume_read)
    , _transfered_data(0)
    , _read_is_stopped(false) {
}

void ProxyPipe::SetListener(std::weak_ptr<ProxyPipeListener> listener) {
  _listener = listener;
}

void ProxyPipe::PushMessage(std::shared_ptr<Message> msg) {
  if(!_src || !_dest) {
    DLOG(error, "Missing one or more clients");
    return;
  }
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&ProxyPipe::PushMessage,
                        shared_from_this(),
                        msg));
    return;
  };
  _active_msg_counter++;
  if((_active_msg_counter >= _stop_read) && !_read_is_stopped) {
    _read_is_stopped = true;
    _src->SetActive(false);
    if(auto listener = _listener.lock()) {
      listener->OnProxyPipePaused();
    }
  }
  _dest->Send(msg);
}

void ProxyPipe::OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&ProxyPipe::OnMsgSent,
                        shared_from_this(),
                        client,
                        msg,
                        success));
    return;
  };
  auto listener = _listener.lock();
  _active_msg_counter--;
  if((_active_msg_counter <= _resume_read) && _read_is_stopped) {
    _read_is_stopped = false;
    _src->SetActive(true);
    if(listener) {
      listener->OnProxyPipeResumed();
    }
  }
  if(!_active_msg_counter && listener) {
    listener->OnProxyPipeEmpty();
  }
}

uint64_t ProxyPipe::GetTransferedDataSize() {
  return _transfered_data;
}
