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

#include "MtlsCppWrapper.h"
#include "Logger.h"

using namespace MtlsCppWrapper;

std::string MtlsCppWrapper::GetErrorName(int error_code) {
  char error_buf[100];
  mbedtls_strerror(error_code, error_buf, 100);
  std::string result(error_buf);
  return result.empty() ? ("Unkonwn error code : " + std::to_string(error_code)) : result;
}

std::shared_ptr<MtlsCppConfig> MtlsCppConfig::CreateServerConfig(const std::string& key,
                                                                const std::string& cert) {
  std::shared_ptr<MtlsCppConfig> result;
  result.reset(new MtlsCppConfig(true));

  if(!result->InitEntropy()) {
    return nullptr;
  }
  if(!result->InitCerts(key, cert)) {
    return nullptr;
  }
  if(!result->InitConfig()) {
    return nullptr;
  }
  return result;
}

std::shared_ptr<MtlsCppConfig> MtlsCppConfig::CreateServerConfig(const std::string& key,
                                                          const std::string& cert,
                                                          const std::string& ca_chain) {
  std::shared_ptr<MtlsCppConfig> result = CreateServerConfig(key, cert);
  if(!result) {
    return nullptr;
  }
  if(!result->InitCaChain(ca_chain)) {
    return nullptr;
  }
  return result;
}

std::shared_ptr<MtlsCppConfig> MtlsCppConfig::CreateClientConfig() {
  std::shared_ptr<MtlsCppConfig> result;
  result.reset(new MtlsCppConfig(false));

  if(!result->InitEntropy()) {
    return nullptr;
  }

  if(!result->InitConfig()) {
    return nullptr;
  }
  return result;
}

std::shared_ptr<MtlsCppConfig> MtlsCppConfig::CreateClientConfig(const std::string& key, const std::string& cert) {
  std::shared_ptr<MtlsCppConfig> result = CreateClientConfig();
  if(!result) {
    return nullptr;
  }
  if(!result->InitCerts(key, cert)) {
    return nullptr;
  }
  return result;
}

std::shared_ptr<MtlsCppConfig> MtlsCppConfig::CreateClientConfig(const std::string& key, 
                                                              const std::string& cert,
                                                              const std::string& ca_chain) {
  std::shared_ptr<MtlsCppConfig> result = CreateClientConfig(key, cert);
  if(!result) {
    return nullptr;
  }
  if(!result->InitCaChain(ca_chain)) {
    return nullptr;
  }
  return result;
}

const mbedtls_ssl_config* MtlsCppConfig::GetRawConfig() {
  return &_ssl_conf;
}

bool MtlsCppConfig::IsServerConfig() {
  return _is_server_config;
}

MtlsCppConfig::MtlsCppConfig(bool is_server_config)
    : _is_server_config(is_server_config) {
  mbedtls_ssl_config_init(&_ssl_conf);
  mbedtls_x509_crt_init(&_owned_cert);
  mbedtls_x509_crt_init(&_ca_chain);
  mbedtls_entropy_init(&_entropy);
  mbedtls_ctr_drbg_init(&_ctr_drbg);
  mbedtls_pk_init(&_pk_context);
}

MtlsCppConfig::~MtlsCppConfig() {
  mbedtls_ssl_config_free(&_ssl_conf);
  mbedtls_x509_crt_free(&_owned_cert);
  mbedtls_x509_crt_free(&_ca_chain);
  mbedtls_entropy_free(&_entropy);
  mbedtls_ctr_drbg_free(&_ctr_drbg);
  mbedtls_pk_free(&_pk_context);
}

bool MtlsCppConfig::InitEntropy() {
  int err = mbedtls_ctr_drbg_seed(&_ctr_drbg,
                          mbedtls_entropy_func,
                          &_entropy,
                          nullptr,
                          0);
  if(err != 0) {
    DLOG(error, "MtlsCppWrapper : Fail mbedtls_ctr_drbg_seed : {}", GetErrorName(err));
    return false;
  }
  return true;
}

bool MtlsCppConfig::InitCerts(const std::string& key, const std::string& cert) {
  int err = mbedtls_pk_parse_key(&_pk_context,
                    (const unsigned char*) key.c_str(),
                    key.length() + 1,
                    nullptr,
                    0,
                    mbedtls_ctr_drbg_random,
                    &_ctr_drbg);
  if(err) {
    DLOG(error, "MtlsCppWrapper : Fail mbedtls_pk_parse_key : {}", GetErrorName(err));
    return false;
  }

  err = mbedtls_x509_crt_parse(&_owned_cert,
                            (const unsigned char*)cert.c_str(),
                            cert.length() + 1);
  if(err) {
    DLOG(error, "MtlsCppWrapper : Fail mbedtls_x509_crt_parse : {}", GetErrorName(err));
    return false;
  }
  err = mbedtls_ssl_conf_own_cert(&_ssl_conf, &_owned_cert, &_pk_context);
  if(err) {
    DLOG(error, "MtlsCppWrapper : Fail mbedtls_ssl_conf_own_cert : {}", GetErrorName(err));
    return false;
  }
  return true;
}

bool MtlsCppConfig::InitCaChain(const std::string& ca_chain) {
  int err = mbedtls_x509_crt_parse(&_ca_chain,
                            (const unsigned char*)ca_chain.c_str(),
                            ca_chain.length() + 1);
  if(err) {
    DLOG(error, "MtlsCppWrapper : Fail mbedtls_x509_crt_parse : {}", GetErrorName(err));
    return false;
  }

  mbedtls_ssl_conf_ca_chain(&_ssl_conf, &_ca_chain, nullptr);
  return true;
}

bool MtlsCppConfig::InitConfig() {
  int err = mbedtls_ssl_config_defaults(&_ssl_conf,
                              _is_server_config ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT,
                              MBEDTLS_SSL_TRANSPORT_STREAM,
                              MBEDTLS_SSL_PRESET_DEFAULT);
  if(err) {
    DLOG(error, "MtlsCppWrapper : Fail mbedtls_ssl_config_defaults : {}", GetErrorName(err));
    return false;
  }
  mbedtls_ssl_conf_rng(&_ssl_conf, mbedtls_ctr_drbg_random, &_ctr_drbg);
  mbedtls_ssl_conf_authmode(&_ssl_conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
  return true;
}

/////////////////////////

std::shared_ptr<MtlsCppSsl> MtlsCppSsl::CreateSsl(int socket_fd, std::shared_ptr<MtlsCppConfig> config) {
  std::shared_ptr<MtlsCppSsl> result;
  result.reset(new MtlsCppSsl());
  if(!result->Init(socket_fd, config)) {
    return nullptr;
  }
  return result;
}

std::shared_ptr<MtlsCppSsl> MtlsCppSsl::CreateSsl(int socket_fd,
                                            std::shared_ptr<MtlsCppConfig> config,
                                            const std::string& host_name) {
  std::shared_ptr<MtlsCppSsl> result = CreateSsl(socket_fd, config);
  if(!result) {
    return nullptr;
  }
  if(!result->SetHostname(host_name)) {
    return nullptr;
  }
  return result;
}

MtlsCppSsl::MtlsCppSsl() {
  mbedtls_ssl_init(&_ssl_context);
  mbedtls_net_init(&_net_context);
}

MtlsCppSsl::~MtlsCppSsl() {
  mbedtls_ssl_free(&_ssl_context);
  mbedtls_net_free(&_net_context);
}

bool MtlsCppSsl::Init(int socket_fd, std::shared_ptr<MtlsCppConfig> config) {
  int err = mbedtls_ssl_setup(&_ssl_context, config->GetRawConfig());
  if(err) {
    return false;
  }
  _net_context.fd = socket_fd;
  mbedtls_ssl_set_bio(&_ssl_context, &_net_context, mbedtls_net_send, mbedtls_net_recv, nullptr);
  return true;
}

bool MtlsCppSsl::SetHostname(const std::string& host_name) {
  int err = mbedtls_ssl_set_hostname(&_ssl_context, host_name.c_str());
  if(err) {
    DLOG(error, "MtlsCppWrapper : Fail mbedtls_ssl_set_hostname : {}", GetErrorName(err));
    return false;
  }
  return true;
}

NetError MtlsCppSsl::MakeHandshake(bool& out_verified) {
  int err = mbedtls_ssl_handshake(&_ssl_context);
  if(err) {
    if(err != MBEDTLS_ERR_SSL_WANT_READ && err != MBEDTLS_ERR_SSL_WANT_WRITE) {
      DLOG(error, "MtlsCppWrapper : Socket : {}, Handshake FAILED : {}", 
            _net_context.fd,
            GetErrorName(err));
      return NetError::FAILED;
    } else {
      return (err == MBEDTLS_ERR_SSL_WANT_READ) ? NetError::NEEDS_READ : NetError::NEEDS_WRITE;
    }
  }

  out_verified = (mbedtls_ssl_get_verify_result(&_ssl_context) == 0);
  return NetError::OK;
}

bool MtlsCppSsl::Read(void* dest, int dest_size, size_t& out_read_size) {
  int read_value = mbedtls_ssl_read(&_ssl_context, (unsigned char*)dest, dest_size);

  if((read_value == MBEDTLS_ERR_SSL_WANT_READ) || (read_value == MBEDTLS_ERR_SSL_WANT_WRITE)) {
    out_read_size = 0;
    return true;
  } else if(read_value < 0) {
    DLOG(warn, "MtlsCppWrapper : mbedtls_ssl_read FAILED : {}", GetErrorName(read_value));
    return false;
  }
  out_read_size = (size_t)read_value;
  return (read_value > 0);
}

bool MtlsCppSsl::Write(const void* buffer, int size, size_t& out_write_size) {
  int result = mbedtls_ssl_write(&_ssl_context, (const unsigned char*)buffer, size);
  if( result<= 0) {
    out_write_size = 0;
    if( result == MBEDTLS_ERR_SSL_WANT_READ ||
        result == MBEDTLS_ERR_SSL_WANT_WRITE ) {
      return true;
    } else {
      DLOG(warn, "MtlsCppWrapper : mbedtls_ssl_write FAILED : {}", GetErrorName(result));
      return false;
    }
  }
  out_write_size = (size_t)result;
  return true;
}