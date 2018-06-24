#include "ConnectionChecker.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctime>

#include "Connection.h"
#include "Logger.h"

time_t CurrentTime() {
  time_t timer;
  time(&timer);
  return timer;
}

ConnectionChecker::ConnectionChecker(std::weak_ptr<ConnectionKeeper> keeper,
                  std::shared_ptr<Connection> connection,
                  size_t check_interval,
                  const std::string& server_url,
                  int server_port)
  : _keeper(keeper)
  , _connection(connection)
  , _check_interval(check_interval)
  , _server_url(server_url)
  , _server_port(server_port)
  , _last_alive(0)
  , _status(NOT_CONNECTED) {
}

ConnectionChecker::~ConnectionChecker() {
  _alive_check.Stop();
  _alive_check.Join();
}

void ConnectionChecker::Init() {
    _alive_check.Run(shared_from_this());
}

void ConnectionChecker::SetStatus(ConnectionChecker::ConnectionStatus status) {
  if(_status != status) {
    switch (status) {
      case NOT_CONNECTED:
        DLOG(info, "ConnectStatus: NOT_CONNECTED");
        break;
      case CONNECTED:
        DLOG(info, "ConnectStatus: CONNECTED");
        break;
      case MAYBE_CONNECTED:
        DLOG(info, "ConnectStatus: MAYBE_CONNECTED");
        break;
      default:
        break;
    }
  }
  _status = status;
}

void ConnectionChecker::OnThreadStarted(int id){
  while(_alive_check.ShouldRun()) {
    DoConnectionCheck();
    sleep(_check_interval);
  }
}

void ConnectionChecker::DoConnectionCheck() {
  time_t time = CurrentTime();
  if(time > _last_alive + _check_interval) {
    ConnectionStatus status = _status;
    switch (status) {
      case NOT_CONNECTED:
      case MAYBE_CONNECTED:
        TryConnect();
        break;
      case CONNECTED:
        SetStatus(MAYBE_CONNECTED);
        if(auto keeper = _keeper.lock())
          keeper->SendPing();
        break;
      default:
        break;
    }
  }
}

void ConnectionChecker::TryConnect() {
  std::shared_ptr<ConnectionKeeper> keeper;
  if(keeper = _keeper.lock())
    keeper->OnDisconnected();
  else
    return;

  auto client = _connection->CreateClient(_server_port, _server_url);
  if(client) {
    SetStatus(CONNECTED);
    keeper->OnConnected(client);
  }
  else {
    SetStatus(NOT_CONNECTED);
  }
}

void ConnectionChecker::Wake() {
  _last_alive = CurrentTime();
  SetStatus(CONNECTED);
}
