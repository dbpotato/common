#[[
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
]]

cmake_minimum_required(VERSION 3.2)
project(UrlLoaderExample)

set(CXX_FLAGS
 "-O2 \
 -std=gnu++11 \
 -Wall \
 -Werror \
 -Wundef \
 -Wcast-align \
 -Wcast-qual \
 -Wno-unused \
 -Wno-delete-non-virtual-dtor"
)

#add_definitions(-DENABLE_DEBUG_LOGGER)
set(ENABLE_SSL false)

set(CMAKE_CXX_FLAGS "${CXX_FLAGS}")
set(CMAKE_SYSTEM_NAME linux)
set(DEFAULT_CXX "g++")

set(COMMON_DIR "${PROJECT_SOURCE_DIR}/../..")
set(SRC_DIR "${PROJECT_SOURCE_DIR}")

#[[
When using ssl provide correct path to mbedtls directory
]]
set(MBEDTLS_DIR "")


if(DEFINED ENV{CXX})
  message("Using user's compiler : " $ENV{CXX})
  set(CMAKE_CXX_COMPILER $ENV{CXX})
else(DEFINED ENV{CXX})
  message("Using default compiler : " ${DEFAULT_CXX})
  set(CMAKE_CXX_COMPILER ${DEFAULT_CXX})
endif(DEFINED ENV{CXX})


set(LIBS
  pthread
  dl
)

set(INCLUDE_DIR
  ${COMMON_DIR}/third_party/spdlog/include
  ${COMMON_DIR}/tools/thread
  ${COMMON_DIR}/tools/logger
  ${COMMON_DIR}/tools/utils
  ${COMMON_DIR}/tools/net
  ${COMMON_DIR}/tools/net/http
)

set(COMMON
  ${COMMON_DIR}/tools/thread/PosixThread.cpp
  ${COMMON_DIR}/tools/thread/ThreadLoop.cpp
  ${COMMON_DIR}/tools/utils/Data.cpp
  ${COMMON_DIR}/tools/utils/DataResource.cpp
  ${COMMON_DIR}/tools/utils/TapeCutter.cpp
  ${COMMON_DIR}/tools/net/Server.cpp
  ${COMMON_DIR}/tools/net/SocketContext.cpp
  ${COMMON_DIR}/tools/net/SocketObject.cpp
  ${COMMON_DIR}/tools/net/Client.cpp
  ${COMMON_DIR}/tools/net/Connection.cpp
  ${COMMON_DIR}/tools/net/ConnectThread.cpp
  ${COMMON_DIR}/tools/net/Epool.cpp
  ${COMMON_DIR}/tools/net/Message.cpp
  ${COMMON_DIR}/tools/net/http/HttpHeader.cpp
  ${COMMON_DIR}/tools/net/http/HttpMessage.cpp
  ${COMMON_DIR}/tools/net/http/HttpDataCutter.cpp
  ${COMMON_DIR}/tools/net/http/HttpDataParser.cpp
  ${COMMON_DIR}/tools/net/http/HttpMessageBuilder.cpp
  ${COMMON_DIR}/tools/net/http/MimeTypeFinder.cpp
)

if(ENABLE_SSL)
  add_definitions(-DENABLE_SSL)
  set(LIBS ${LIBS} mbedtls mbedcrypto mbedx509)
  set(INCLUDE_DIR ${INCLUDE_DIR} ${MBEDTLS_DIR}/include)
  set(COMMON
    ${COMMON}
    ${COMMON_DIR}/tools/net/ConnectionMTls.cpp
    ${COMMON_DIR}/tools/net/MtlsCppWrapper.cpp
  )
  link_directories(
    ${MBEDTLS_DIR}/lib
  )
endif(ENABLE_SSL)

include_directories(
  ${INCLUDE_DIR}
)


set(LOADER
  ${COMMON}
  ${SRC_DIR}/main.cpp

)

add_executable(loader ${LOADER})
target_link_libraries(loader ${LIBS})
