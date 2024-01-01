/*
Copyright (c) 2018-2024 Adam Kaniewski

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

#include "ConnectionChecker.h"
#include "ThreadLoop.h"


static const MonitorTask::uint32_seconds RECHECK_TIME{2};
static const MonitorTask::uint32_seconds INACTIVITY_TIME{8};

std::weak_ptr<ConnectionChecker> ConnectionChecker::_instance;


MonitorTask::uint32_time_point MonitorTask::GetLastReadTime() {
  auto seconds = _last_read_time_sec.load();
  return uint32_time_point(uint32_seconds(seconds));
}

MonitorTask::uint32_time_point MonitorTask::GetCurrentTime() {
  return std::chrono::time_point_cast<MonitorTask::uint32_seconds>(std::chrono::steady_clock::now());
}

void MonitorTask::UpdateLastReadTime() {
 auto seconds = GetCurrentTime().time_since_epoch();
 _last_read_time_sec.store(seconds.count());
}

MonitorTask::MonitorTask(std::weak_ptr<Client> client, std::weak_ptr<MonitoringManager> manager, std::shared_ptr<ConnectionChecker> checker)
    : _state(ConnectionState::CONNECTED)
    , _port(-1)
    , _client(client)
    , _manager(manager)
    , _checker(checker)
    , _last_read_time_sec(0) {
}

MonitorTask::MonitorTask(const std::string& url, int port, std::weak_ptr<MonitoringManager> manager, std::shared_ptr<ConnectionChecker> checker)
    : _state(ConnectionState::NOT_CONNECTED)
    , _url(url)
    , _port(port)
    , _manager(manager)
    , _checker(checker)
    , _last_read_time_sec(0) {
}

bool MonitorTask::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  auto manager_sptr = _manager.lock();
  if(!manager_sptr) {
    return false;
  }

  client->SetManager(manager_sptr);
  client->AddListener(shared_from_this());

  if(err != NetError::OK) {
    SetState(ConnectionState::NOT_CONNECTED);
    return manager_sptr->OnClientConnecting(client, err);
  }

  if(manager_sptr->OnClientConnecting(client, err)) {
   _client = client;
   SetState(ConnectionState::CONNECTED);
   return true;
  }

  SetState(ConnectionState::NOT_CONNECTED);
  return false;
}

void MonitorTask::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  UpdateLastReadTime();
}

void MonitorTask::OnClientClosed(std::shared_ptr<Client> client) {
  _client = {};
}

void MonitorTask::RequestCreatingClient() {
  auto manager_sptr = _manager.lock();
  if(manager_sptr) {
    _client = {};
    SetState(ConnectionState::CONNECTING);
    manager_sptr->CreateClient(shared_from_this(), _url, _port);
  }
}

void MonitorTask::SetState(ConnectionState new_state) {
  _state.store(new_state);
}

bool MonitorTask::Check() {
  auto manager_sptr = _manager.lock();
  if(!manager_sptr) {
    return false;
  }

  if(_port > -1) {
    CheckForReconnectingClient();
    return true;
  }

  return CheckForDisconnectingClient();
}

void MonitorTask::CheckForReconnectingClient() {
  auto current_state = _state.load();
  auto client_sptr = _client.lock();
  auto manager_sptr = _manager.lock();

  if(!client_sptr) {
    if(current_state != ConnectionState::CONNECTING) {
      RequestCreatingClient();
    }
    return;
  }

  auto time_since_read = GetCurrentTime() - GetLastReadTime();

  if(time_since_read > INACTIVITY_TIME) {
    if(current_state == ConnectionState::CONNECTED) {
      SetState(ConnectionState::MAYBE_CONNECTED);
      manager_sptr->SendPingToClient(client_sptr);
      return;
    } else if(current_state == ConnectionState::MAYBE_CONNECTED){
      manager_sptr->OnClientUnresponsive(client_sptr);
      RequestCreatingClient();
    }
  } else {
    if(current_state != ConnectionState::CONNECTED) {
      SetState(ConnectionState::CONNECTED);
    }
  }
}

bool MonitorTask::CheckForDisconnectingClient() {
  auto current_state = _state.load();
  auto client_sptr = _client.lock();
  auto manager_sptr = _manager.lock();

  if(!client_sptr) {
    return false;
  }

  auto time_since_read = GetCurrentTime() - GetLastReadTime();

  if(time_since_read > INACTIVITY_TIME) {
    if(current_state == ConnectionState::CONNECTED) {
      SetState(ConnectionState::MAYBE_CONNECTED);
      manager_sptr->SendPingToClient(client_sptr);
      return true;
    } else if(current_state == ConnectionState::MAYBE_CONNECTED){
      manager_sptr->OnClientUnresponsive(client_sptr);
      _client = {};
      return false;
    }
  } else {
    if(current_state != ConnectionState::CONNECTED) {
      SetState(ConnectionState::CONNECTED);
    }
  }
  return true;
}

ConnectionChecker::ConnectionChecker() {
  _thread_loop = std::make_shared<ThreadLoop>();
  _thread_loop->Init();
}

void ConnectionChecker::Init() {
  std::weak_ptr<ConnectionChecker> this_wptr = shared_from_this();
  _thread_loop->Post(std::bind(&ConnectionChecker::MaybeCheckTasks, this_wptr), RECHECK_TIME, true);
}

std::shared_ptr<ConnectionChecker> ConnectionChecker::GetInstance() {
  std::shared_ptr<ConnectionChecker> instance;
  if(!(instance =_instance.lock())) {
    instance.reset(new ConnectionChecker());
    instance->Init();
    _instance = instance;
  }
  return instance;
}

void ConnectionChecker::MointorUrl(const std::string& url, int port, std::weak_ptr<MonitoringManager> owner) {
  auto checker = ConnectionChecker::GetInstance();
  auto task = std::make_shared<MonitorTask>(url, port, owner, checker);
  task->RequestCreatingClient();
  checker->AddTask(task);
}

void ConnectionChecker::MonitorClient(std::shared_ptr<Client> client, std::weak_ptr<MonitoringManager> owner) {
  auto checker = ConnectionChecker::GetInstance();
  auto task = std::make_shared<MonitorTask>(client, owner, checker);
  client->AddListener(task);
  checker->AddTask(task);
}

void ConnectionChecker::AddTask(std::shared_ptr<MonitorTask> task) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&ConnectionChecker::AddTask, shared_from_this(), task));
    return;
  }
  _tasks.push_back(task);
}

void ConnectionChecker::MaybeCheckTasks(std::weak_ptr<ConnectionChecker> instance) {
  auto instance_sptr = instance.lock();
  if(instance_sptr) {
    instance_sptr->CheckTasks();
  }
}

void ConnectionChecker::CheckTasks() {
  for(auto it = _tasks.begin(); it != _tasks.end();) {
    std::shared_ptr<MonitorTask> task = *it;
    bool keep_task = task->Check();
    if(!keep_task){
      it = _tasks.erase(it);
    } else {
      ++it;
    }
  }
}
