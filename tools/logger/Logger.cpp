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

#include "Logger.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/pattern_formatter.h>

#ifdef __ANDROID__
#include <android/log.h>

static const std::string DEFAULT_LOGGER_PATTERN  = "%v";
static const std::string DEFAULT_ANDROID_LOG_TAG = "Logger";
#else
static const std::string DEFAULT_LOGGER_PATTERN = "[%H:%M:%S:%e] [%t] [%l] %v";
#endif

std::shared_ptr<spdlog::sinks::basic_file_sink_mt> CreateFileSink(const std::string& log_file_path) {
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file_path, false);
  return file_sink;
}

Logger::Level SpdlogLevelToLoogerLevel(spdlog::level::level_enum level) {
  switch(level) {
    case spdlog::level::trace:
      return Logger::Level::TRACE;
    case spdlog::level::debug:
      return Logger::Level::DEBUG;
    case spdlog::level::info:
      return Logger::Level::INFO;
    case spdlog::level::warn:
      return Logger::Level::WARN;
    case spdlog::level::err:
      return Logger::Level::ERROR;
    case spdlog::level::critical:
      return Logger::Level::CRITICAL;
    default:
      return Logger::Level::INFO;
  };
}

class func_sink : public spdlog::sinks::sink {
public:
  func_sink():
      _log_func(nullptr),
      _log_func_arg(nullptr),
      _formatter(nullptr) {
    _formatter = spdlog::details::make_unique<spdlog::pattern_formatter>();
  }

  virtual ~func_sink(){;}

  void SetLogFunc(void(*func)(Logger::Level, const std::string&, void*), void* arg){
    _log_func = func;
    _log_func_arg = arg;
  }

  void log(const spdlog::details::log_msg& msg) override {
    std::lock_guard<std::mutex> lock(_mutex);

    spdlog::memory_buf_t formatted;
    _formatter->format(msg, formatted);
    std::string formatted_msg = std::string(formatted.data(), formatted.size());

    if(_log_func) {
      if(formatted_msg.size()) {
        _log_func(SpdlogLevelToLoogerLevel(msg.level),
                                          formatted_msg.substr(0, formatted_msg.size() - 1),
                                          _log_func_arg);
      }
    }
    else{
#ifdef __ANDROID__
      __android_log_print(android_log_level(msg.level),
                          DEFAULT_ANDROID_LOG_TAG,
                          "%s",
                          formatted_msg.substr(0, formatted_msg.size() - 1).c_str());
#else
      fwrite(formatted_msg.c_str(), sizeof(char), formatted_msg.size(), stdout);
      flush();
#endif
    }
  }
  void set_pattern(const std::string &pattern) override {
    std::lock_guard<std::mutex> lock(_mutex);
    set_pattern_int(pattern);
  }

  void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override {
    std::lock_guard<std::mutex> lock(_mutex);
    set_formatter_int(std::move(sink_formatter));
  }
protected:
  void flush() override {
    fflush(stdout);
  }

  void set_pattern_int(const std::string &pattern){
    set_formatter_int(spdlog::details::make_unique<spdlog::pattern_formatter>(pattern));
  }

  void set_formatter_int(std::unique_ptr<spdlog::formatter> sink_formatter) {
    _formatter = std::move(sink_formatter);
  }

  spdlog::string_view_t format(const spdlog::details::log_msg &msg) {
    spdlog::memory_buf_t formatted;
    _formatter->format(msg, formatted);
    return spdlog::string_view_t(formatted.data(), formatted.size());
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
  void(*_log_func)(Logger::Level, const std::string&, void*);
  void* _log_func_arg;
  std::unique_ptr<spdlog::formatter> _formatter;
};


Logger::LogWrapper::LogWrapper(std::shared_ptr<spdlog::logger> logger)
    : _internal(logger) {
}

void Logger::LogWrapper::DoLog(Logger::Level level, const std::string& msg) {
  switch (level) {
  case Logger::Level::TRACE:
    _internal->trace(msg);
    break;
  case Logger::Level::DEBUG:
    _internal->debug(msg);
    break;
  case Logger::Level::INFO:
    _internal->info(msg);
    break;
  case Logger::Level::WARN:
    _internal->warn(msg);
    break;
  case Logger::Level::ERROR:
    _internal->error(msg);
    break;
  case Logger::Level::CRITICAL:
    _internal->critical(msg);
    break;
  default:
    break;
  }
}

void Logger::LogWrapper::set_pattern(const std::string& format) {
  _internal->set_pattern(format);
}

Logger& Logger::Instance() {
  static Logger instance;
  return instance;
}

void Logger::SetLogFunc(void(*func)(Logger::Level,
                              const std::string&, void*),
                              int id_looger,
                              void* log_func_arg) {
  auto logger = GetLogger(id_looger)->Internal();
  if(logger) {
    auto sink = std::static_pointer_cast<func_sink>(logger->sinks()[0]);
    sink->SetLogFunc(func, log_func_arg);
  }
}

void Logger::SetPattern(const std::string& pattern, int id_looger) {
  auto logger = GetLogger(id_looger);
  logger->set_pattern(pattern);
}

bool Logger::Init(int id_looger, std::shared_ptr<spdlog::sinks::sink> additional_sink) {
  if(InternalGet(id_looger)) {
    return false;
  }
  InternalInitialize(id_looger, additional_sink);
  return true;
}

bool Logger::Init(int id_looger, const std::string& log_file_path) {
  return Init(id_looger, CreateFileSink(log_file_path));
}

bool Logger::Init(const std::string& log_file_path) {
  return Init(0, log_file_path);
}

std::shared_ptr<Logger::LogWrapper> Logger::GetLogger(int id) {
  auto logger = InternalGet(id);
  if(logger) {
    return logger;
  }
  return InternalInitialize(id);
}

std::shared_ptr<Logger::LogWrapper> Logger::InternalInitialize(int id,
    std::shared_ptr<spdlog::sinks::sink> additional_sink) {
  std::lock_guard<std::mutex> lock(_init_mutex);
  auto recheck = InternalGet(id);
  if(recheck) {
    return recheck;
  }

  auto sink = std::make_shared<func_sink>();
  std::shared_ptr<spdlog::logger> logger;
  logger = std::make_shared<spdlog::logger>(std::to_string(id), std::move(sink));

  if(additional_sink) {
    logger->sinks().push_back(additional_sink);
  }

  logger->set_pattern(DEFAULT_LOGGER_PATTERN);
  logger->flush_on(spdlog::level::info);
  spdlog::details::registry::instance().register_logger(logger);

  auto wrapper = std::make_shared<Logger::LogWrapper>(logger);
  _loggers.insert({id, wrapper});

  if(!id) {
    _default_logger = wrapper;
  }
  return wrapper;
}

std::shared_ptr<Logger::LogWrapper> Logger::InternalGet(int id) {
  if(!id) {
    return _default_logger;
  } else {
    auto it = _loggers.find(id);
    if(it != _loggers.end()) {
      return it->second;
    }
    return {};
  }
}
