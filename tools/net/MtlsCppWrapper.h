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

/*
Based on Mbed TLS 3.4.1
*/

#pragma once

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl_cache.h"
#include "mbedtls/ssl.h"

#include "NetUtils.h"

#include <memory>
#include <string>

namespace MtlsCppWrapper {

class MtlsCppSsl;

static std::string GetErrorName(int error_code);

class MtlsCppConfig {
public:
  static std::shared_ptr<MtlsCppConfig> CreateServerConfig(const std::string& key, const std::string& cert);
  static std::shared_ptr<MtlsCppConfig> CreateServerConfig(const std::string& key,
                                                          const std::string& cert,
                                                          const std::string& ca_chain);

  static std::shared_ptr<MtlsCppConfig> CreateClientConfig();

  static std::shared_ptr<MtlsCppConfig> CreateClientConfig(const std::string& key,
                                                          const std::string& cert);

  static std::shared_ptr<MtlsCppConfig> CreateClientConfig(const std::string& key,
                                                          const std::string& cert,
                                                          const std::string& ca_chain);
  bool IsServerConfig();
  ~MtlsCppConfig();
protected:
  MtlsCppConfig(bool is_server_config);
  bool InitConfig();
  bool InitEntropy();
  bool InitCerts(const std::string& key, const std::string& cert);
  bool InitCaChain(const std::string& ca_chain);
  const mbedtls_ssl_config* GetRawConfig();
  bool _is_server_config;
  mbedtls_ssl_config _ssl_conf;
  mbedtls_x509_crt _owned_cert;
  mbedtls_x509_crt _ca_chain;
  mbedtls_entropy_context _entropy;
  mbedtls_ctr_drbg_context _ctr_drbg;
  mbedtls_pk_context _pk_context;
  friend class MtlsCppSsl;
};

class MtlsCppSsl {
public:
  static std::shared_ptr<MtlsCppSsl> CreateSsl(int socket_fd, std::shared_ptr<MtlsCppConfig> config);
  static std::shared_ptr<MtlsCppSsl> CreateSsl(int socket_fd,
                                              std::shared_ptr<MtlsCppConfig> config,
                                              const std::string& host_name);
  bool SetHostname(const std::string& host_name);
  NetError MakeHandshake(bool& out_verified);
  bool Read(void* dest, int dest_size, size_t& out_read_size);
  bool Write(const void* buffer, int size, size_t& out_write_size);
  ~MtlsCppSsl();
protected:
  MtlsCppSsl();
  bool Init(int socket_fd, std::shared_ptr<MtlsCppConfig> config);
  mbedtls_ssl_context _ssl_context;
  mbedtls_net_context _net_context;
};

} //namespace MtlsCppWrapper

