/*
Copyright (c) 2018 - 2020 Adam Kaniewski

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

#ifdef __ANDROID__
#include <android/log.h>

static const char* DEFAULT_LOGGER_PATTERN  = "%v";
static const char* DEFAULT_ANDROID_LOG_TAG = "Logger";
#else
static const char* DEFAULT_LOGGER_PATTERN = "[%H:%M:%S:%e] [%t] [%l] %v";
#endif


class func_sink : public spdlog::sinks::sink {
public:
  func_sink():
      _log_func(nullptr),
      _log_func_arg(nullptr) {
  }

  virtual ~func_sink(){;}

  void SetLogFunc(void(*func)(spdlog::level::level_enum, const std::string&, void*), void* arg){
    _log_func = func;
    _log_func_arg = arg;
  }

  void log(const spdlog::details::log_msg& msg) override {
    std::lock_guard<std::mutex> lock(_mutex);
    if(_log_func) {
      if(msg.formatted.size())
        _log_func(msg.level, std::string(msg.formatted.data(), msg.formatted.size() - 1), _log_func_arg);
    }
    else{
#ifdef __ANDROID__
      __android_log_print(android_log_level(msg.level),
                          DEFAULT_ANDROID_LOG_TAG,
                          "%s",
                          std::string(msg.formatted.data(), msg.formatted.size() - 1).c_str());
#else
      fwrite(msg.formatted.data(), sizeof(char), msg.formatted.size(), stdout);
      flush();
#endif
    }
  }
protected:
  void flush() override {
    fflush(stdout);
  }
#ifdef __ANDROID__
  android_LogPriority android_log_level(spdlog::level::level_enum level){
    switch (level) {
      case spdlog::level::trace:
        return ANDROID_LOG_VERBOSE;
      case spdlog::level::debug:
        return ANDROID_LOG_DEBUG;
      case spdlog::level::info:
        return ANDROID_LOG_INFO;
      case spdlog::level::warn:
        return ANDROID_LOG_WARN;
      case spdlog::level::err:
        return ANDROID_LOG_ERROR;
      case spdlog::level::critical:
        return ANDROID_LOG_FATAL;
      default:
        return ANDROID_LOG_DEFAULT;
    }
  }
#endif 
  std::mutex _mutex;
  void(*_log_func)(spdlog::level::level_enum, const std::string&, void*);
  void* _log_func_arg;
};



class Logger {
public:
  static Logger& Instance() {
    static Logger instance;
    return instance;
  }

  void SetLogFunc(void(*func)(spdlog::level::level_enum,
                              const std::string&, void*),
                              int id_looger,
                              void* log_func_arg) {
    std::shared_ptr<func_sink> sink = GetSink(id_looger);
    sink->SetLogFunc(func, log_func_arg);
  }

  void SetPattern(const char* pattern, int id_looger = 0) {
    auto logger = GetLogger(id_looger);
    logger->set_pattern(pattern);
  }

  bool Init(int id_looger, std::shared_ptr<spdlog::sinks::sink> additional_sink = nullptr) {
    if(InternalGet(id_looger))
      return false;

    InternalInitialize(id_looger, additional_sink);
    return true;
  }

  std::shared_ptr<spdlog::logger> GetLogger(int id) {
    auto logger = InternalGet(id);
    if(logger)
      return logger;

    return InternalInitialize(id);
  }

  std::shared_ptr<func_sink> GetSink(int logger_id) {
    auto logger = GetLogger(logger_id);
    return std::static_pointer_cast<func_sink>(logger->sinks()[0]);
  }

protected:
  Logger(){}
  Logger(const Logger&){}
  Logger(const Logger&&){}

  std::shared_ptr<spdlog::logger> InternalInitialize(int id,
                                             std::shared_ptr<spdlog::sinks::sink> additional_sink = nullptr) {
    std::lock_guard<std::mutex> lock(_init_mutex);

    auto recheck = InternalGet(id);
    if(recheck)
      return recheck;

    auto sink = std::make_shared<func_sink>();
    std::shared_ptr<spdlog::logger> logger;

    if(additional_sink)
      logger = spdlog::details::registry::instance().create(std::to_string(id), {sink, additional_sink});
    else
      logger = spdlog::details::registry::instance().create(std::to_string(id), sink);

    logger->set_pattern(DEFAULT_LOGGER_PATTERN);
    if(!id)
      _default_logger = logger;

    return logger;
  }

  std::shared_ptr<spdlog::logger> InternalGet(int id) {
    if(!id)
      return _default_logger;
    else
      return spdlog::details::registry::instance().get(std::to_string(id));
  }

  std::shared_ptr<spdlog::logger> _default_logger;
  std::mutex _init_mutex;
};

static std::shared_ptr<spdlog::logger> log(int id = 0) {
  return Logger::Instance().GetLogger(id);
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


static NoSPDLogger* log(int id = 0){
  return &NoSPDLogger::Instance();
}

#endif //DISABLE_SPD_LOGGER

#ifdef ENABLE_DEBUG_LOGGER
  #define DLOG(type, format, ...) log()->type(format, ##__VA_ARGS__)
#else
  #define DLOG(...) do {} while(0)
#endif
