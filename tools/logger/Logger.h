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

#ifndef DISABLE_SPD_LOGGER
#include <spdlog/spdlog.h>

class func_sink : public spdlog::sinks::sink {
public:
  func_sink():
      _log_func(nullptr) {
  }
  virtual ~func_sink(){;}
  static std::shared_ptr<func_sink> instance() {
    static std::shared_ptr<func_sink> instance = std::make_shared<func_sink>();
    return instance;
  }
  void SetLogFunc(void(*func)(spdlog::level::level_enum, const std::string&)){
    _log_func = func;
  }
  void log(const spdlog::details::log_msg& msg) override {
    std::lock_guard<std::mutex> lock(_mutex);
    if(_log_func)
      _log_func(msg.level, std::string(msg.formatted.data(), msg.formatted.size()));
    else{
      fwrite(msg.formatted.data(), sizeof(char), msg.formatted.size(), stdout);
      flush();
    }
  }
protected:
  void flush() override {
    fflush(stdout);
  }

  std::mutex _mutex;
  void(*_log_func)(spdlog::level::level_enum lv, const std::string&);
};

class Logger {
public:
  Logger() {
    _logger = spdlog::details::registry::instance().create("console", func_sink::instance());
    _logger->set_pattern("[%H:%M:%S:%e] [%t] [%l] %v");
#ifdef ENABLE_DEBUG_LOGGER
    _logger->set_level(spdlog::level::debug);
#endif
  }
  static Logger& Instance() {
    static Logger instance;
    return instance;
  }
  void SetLogFunc(void(*func)(spdlog::level::level_enum, const std::string&)) {
    func_sink::instance()->SetLogFunc(func);
  }

  std::shared_ptr<spdlog::logger> _logger;
};

static std::shared_ptr<spdlog::logger> log(){
  return Logger::Instance()._logger;
}

#else

class NoSPDLogger {
protected:
  NoSPDLogger() {
    printf("Using dummy logger!\n");
  }
public:
  static NoSPDLogger& Instance() {
    static NoSPDLogger instance;
    return instance;
  }
  void warn(const std::string& format, ...){}
  void info(const std::string& format, ...){}
  void debug(const std::string& format, ...){}
  void error(const std::string& format, ...){}
  void set_pattern(const std::string& format){}
};


static NoSPDLogger* log(){
  return &NoSPDLogger::Instance();
}

#endif //DISABLE_SPD_LOGGER

#ifdef ENABLE_DEBUG_LOGGER
  #define DLOG(type, format, ...) log()->type(format, ##__VA_ARGS__)
#else
  #define DLOG(...) {}
#endif
