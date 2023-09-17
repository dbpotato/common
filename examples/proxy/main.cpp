/*
Copyright (c) 2023 Adam Kaniewski

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

#include "Proxy.h"
#include "StringUtils.h"
#include "Logger.h"

#include <memory>
#include <string>

int main(int argc, char** args) {
  int listen_port = -1;
  int host_port = -1;
  std::string host_url;
  if(argc == 4) {
    host_url = std::string(args[2]);
    StringUtils::ToInt(args[3], host_port);
    StringUtils::ToInt(args[1], listen_port);
    auto proxy = std::make_shared<Proxy>(listen_port, host_url, host_port);
    if(!proxy->Init()) {
      return 0;
    } else{
      log()->info("Proxy started");
    }
    while(true) {
      sleep(1);
    }
  } else {
    log()->error("Usage : proxy <listen port> <host url> <host port>");
    return 0;
  }
  return 0;
}