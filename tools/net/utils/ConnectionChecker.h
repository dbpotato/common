/*
Copyright (c) 2018 - 2023 Adam Kaniewski

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

#include "Client.h"

#include <string>
#include <memory>
#include <chrono>
#include <unistd.h>

class MonitorTask;
class ThreadLoop;
class ConnectionChecker;

class MonitoringManager : public ClientManager {
public:
  virtual void SendPingToClient(std::shared_ptr<Client> client) = 0;
  virtual void CreateClient(std::shared_ptr<MonitorTask> task, const std::string& url, int port) = 0;
  virtual void OnClientUnresponsive(std::shared_ptr<Client> client) = 0;
};

class MonitorTask : public ClientManager
                  , public std::enable_shared_from_this<MonitorTask>{
public:
  using uint32_seconds = std::chrono::duration<uint32_t>;
  using uint32_time_point = std::chrono::time_point<std::chrono::steady_clock, uint32_seconds>;

  enum ConnectionState{
    NOT_CONNECTED = 0,
    CONNECTING,
    CONNECTED,
    MAYBE_CONNECTED
  };
  MonitorTask(std::weak_ptr<Client> client, std::weak_ptr<MonitoringManager> manager, std::shared_ptr<ConnectionChecker> checker);
  MonitorTask(const std::string& url, int port, std::weak_ptr<MonitoringManager> manager, std::shared_ptr<ConnectionChecker> checker);
  bool Check();
  void RequestCreatingClient();

  bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  void OnClientClosed(std::shared_ptr<Client> client) override;

private:
  void CheckForReconnectingClient();
  bool CheckForDisconnectingClient();
  void SetState(ConnectionState new_state);
  void UpdateLastReadTime();
  uint32_time_point GetCurrentTime();
  uint32_time_point GetLastReadTime();

  std::atomic<ConnectionState> _state;
  std::string _url;
  int _port;
  std::weak_ptr<Client> _client;
  std::weak_ptr<MonitoringManager> _manager;
  std::shared_ptr<ConnectionChecker> _checker;
  std::atomic_uint32_t _last_read_time_sec;
};


class ConnectionChecker : public std::enable_shared_from_this<ConnectionChecker> {
public:
  static void MointorUrl(const std::string& url, int port, std::weak_ptr<MonitoringManager> owner);
  static void MonitorClient(std::shared_ptr<Client> client, std::weak_ptr<MonitoringManager> owner);
  static void MaybeCheckTasks(std::weak_ptr<ConnectionChecker> instance);
protected :
  static std::shared_ptr<ConnectionChecker> GetInstance();
  ConnectionChecker();
  void Init();
  void CheckTasks();
  void AddTask(std::shared_ptr<MonitorTask> task);
  static std::weak_ptr<ConnectionChecker> _instance;
private :
  std::vector<std::shared_ptr<MonitorTask>> _tasks;
  std::shared_ptr<ThreadLoop> _thread_loop;
};
