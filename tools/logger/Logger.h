/*
Copyright (c) 2018 - 2024 Adam Kaniewski

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


#include <string>
#include <memory>
#include <mutex>
#include <map>

#define FMT_HEADER_ONLY
#include <spdlog/fmt/bundled/format.h>

namespace spdlog {
  class logger;
  namespace sinks {
    class sink;
  }
};

class Logger {
public:
  enum Level{
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL
  };

  class LogWrapper {
  private:
    void DoLog(Logger::Level level, const std::string& msg);
  public:
    LogWrapper(std::shared_ptr<spdlog::logger> logger);

    template <typename... Args>
    void trace(const std::string& format, Args&&... args) {
      std::string formated = fmt::format(format, std::forward<Args>(args)...);
      DoLog(Logger::Level::TRACE, formated);
    }

    template <typename... Args>
    void debug(const std::string& format, Args&&... args) {
      std::string formated = fmt::format(format, std::forward<Args>(args)...);
      DoLog(Logger::Level::DEBUG, formated);
    }

    template <typename... Args>
    void info(const std::string& format, Args&&... args) {
      std::string formated = fmt::format(format, std::forward<Args>(args)...);
      DoLog(Logger::Level::INFO, formated);
    }

    template <typename... Args>
    void warn(const std::string& format, Args&&... args) {
      std::string formated = fmt::format(format, std::forward<Args>(args)...);
      DoLog(Logger::Level::WARN, formated);
    }

    template <typename... Args>
    void error(const std::string& format, Args&&... args) {
      std::string formated = fmt::format(format, std::forward<Args>(args)...);
      DoLog(Logger::Level::ERROR, formated);
    }

    template <typename... Args>
    void critical(const std::string& format, Args&&... args) {
      std::string formated = fmt::format(format, std::forward<Args>(args)...);
      DoLog(Logger::Level::CRITICAL, formated);
    }

    void set_pattern(const std::string& format);
    std::shared_ptr<spdlog::logger> Internal() {return _internal;}
  private:
    std::shared_ptr<spdlog::logger> _internal;
  };

  static Logger& Instance();
  void SetLogFunc(void(*func)(Logger::Level,
                              const std::string&, void*),
                              int id_looger,
                              void* log_func_arg);

  void SetPattern(const std::string& pattern, int id_looger = 0);

  bool Init(int id_looger, std::shared_ptr<spdlog::sinks::sink> additional_sink = nullptr);
  bool Init(int id_looger, const std::string& log_file_path);
  bool Init(const std::string& log_file_path);
  std::shared_ptr<LogWrapper> GetLogger(int id);

protected:
  Logger(){}
  Logger(const Logger&){}
  Logger(const Logger&&){}

  std::shared_ptr<LogWrapper> InternalInitialize(int id,
      std::shared_ptr<spdlog::sinks::sink> additional_sink = nullptr);
  std::shared_ptr<LogWrapper> InternalGet(int id);

  std::map<int, std::shared_ptr<LogWrapper>> _loggers;
  std::shared_ptr<LogWrapper> _default_logger;
  std::mutex _init_mutex;
};

static std::shared_ptr<Logger::LogWrapper> log(int id = 0) {
  return Logger::Instance().GetLogger(id);
};

#ifdef ENABLE_DEBUG_LOGGER
  #define DLOG_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
  #define DLOG_FILENAME_LINE ("[" + std::string(DLOG_FILENAME) + ":" + std::to_string(__LINE__) + "]")
  #define DLOG(type, format, ...) log()->type(std::string(DLOG_FILENAME_LINE) + " : " + format, ##__VA_ARGS__)
#else
  #define DLOG(...) do {} while(0)
#endif
