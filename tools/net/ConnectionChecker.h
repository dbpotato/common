#pragma once

#include <string>
#include <memory>
#include <atomic>

#include "PosixThread.h"

class Connection;
class Client;

class ConnectionKeeper {
public :
  virtual void OnConnected(std::shared_ptr<Client> client) = 0;
  virtual void OnDisconnected() = 0;
  virtual void SendPing() = 0;
};

class ConnectionChecker : public ThreadObject, public std::enable_shared_from_this<ConnectionChecker> {
public:
  enum ConnectionStatus{
    NOT_CONNECTED,
    CONNECTED,
    MAYBE_CONNECTED
  };
  ConnectionChecker(std::weak_ptr<ConnectionKeeper> keeper,
                    std::shared_ptr<Connection> connection,
                    size_t check_interval,
                    const std::string& server_url,
                    int server_port);

  ~ConnectionChecker();
  void Init();
  void Wake();
  //ThreadObject
  void OnThreadStarted(int thread_id) override;

protected :
  static void* StartRunThread(void*);
  //void Run();
  void DoConnectionCheck();
  void TryConnect();
  void SetStatus(ConnectionStatus status);
  std::shared_ptr<Connection> _connection;
  PosixThread _alive_check;
  std::weak_ptr<ConnectionKeeper> _keeper;
  size_t _check_interval;
  std::string _server_url;
  int _server_port;
  std::atomic<time_t> _last_alive;
  std::atomic<ConnectionStatus> _status;
};
