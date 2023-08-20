/*
Copyright (c) 2022 Adam Kaniewski

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

#include "StringUtils.h"

#include <map>
#include <string>


namespace HttpHeaderProtocol {
  enum Type {
    UNKNOWN_TYPE,
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2
  };

  static const std::map<std::string, Type> lowercase_protocol_str_to_enum = {
    {"http/1.0", Type::HTTP_1_0},
    {"http/1.1", Type::HTTP_1_1},
    {"http/2", Type::HTTP_2},
  };

  static const std::map<Type, std::string> enum_to_protocol_str = {
    {Type::HTTP_1_0, "HTTP/2"},
    {Type::HTTP_1_1,"HTTP/1.1"},
    {Type::HTTP_2, "HTTP/2"}
  };

  static bool GetTypeFromString(std::string type_str, Type& type) {
    auto it = lowercase_protocol_str_to_enum.find(StringUtils::Lowercase(type_str));
    if(it == lowercase_protocol_str_to_enum.end())
      return false;

    type = it->second;
    return true;
  };

  static std::string GetStringFromType(Type type) {
    auto it = enum_to_protocol_str.find(type);
    return it->second;
  };
};

namespace HttpHeaderMethod {
  enum Type {
    UNKNOWN_TYPE,
    CONNECT,
    DELETE,
    GET,
    HEAD,
    OPTIONS,
    PATCH,
    POST,
    PUT,
    TRACE,

    //WebDAV
    COPY,
    LOCK,
    MKCOL,
    MOVE,
    PROPFIND,
    PROPPATCH,
    UNLOCK
  };

  static const std::map<std::string, Type> lowercase_method_str_to_enum = {
    {"connect", Type::CONNECT},
    {"delete", Type::DELETE},
    {"get", Type::GET},
    {"head", Type::HEAD},
    {"options", Type::OPTIONS},
    {"patch", Type::PATCH},
    {"post", Type::POST},
    {"put", Type::PUT},
    {"trace", Type::TRACE},

    {"copy", Type::COPY},
    {"lock", Type::LOCK},
    {"mkcol", Type::MKCOL},
    {"move", Type::MOVE},
    {"propfind", Type::PROPFIND},
    {"proppatch", Type::PROPPATCH},
    {"unlock", Type::UNLOCK}
  };

  static const std::map<Type, std::string> enum_to_method_str = {
    {Type::CONNECT, "CONNECT"},
    {Type::DELETE, "DELETE"},
    {Type::GET, "GET"},
    {Type::HEAD, "HEAD"},
    {Type::OPTIONS, "OPTIONS"},
    {Type::PATCH, "PATCH"},
    {Type::POST, "POST"},
    {Type::PROPFIND, "PROPFIND"},
    {Type::PUT, "PUT"},
    {Type::TRACE, "TRACE"},

    {Type::COPY, "COPY"},
    {Type::LOCK, "LOCK"},
    {Type::MKCOL, "MKCOL"},
    {Type::MOVE, "MOVE"},
    {Type::PROPFIND, "PROPFIND"},
    {Type::PROPPATCH, "PROPPATCH"},
    {Type::UNLOCK, "UNLOCK"}
  };

  static bool GetTypeFromString(std::string type_str, Type& type) {
    auto it = lowercase_method_str_to_enum.find(StringUtils::Lowercase(type_str));
    if(it == lowercase_method_str_to_enum.end()) {
      return false;
    }

    type = it->second;
    return true;
  };

  static std::string GetStringFromType(Type type) {
    auto it = enum_to_method_str.find(type);
    return it->second;
  };
};

namespace HttpHeaderStatus {
  static const std::map<int, std::string> int_to_status_str = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {103, "Early Hints"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a teapot"},
    {422, "Unprocessable Entity"},
    {425, "Too Early"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"}
  };

  static bool GetStringFromCode(int code, std::string& code_str) {
    auto it = int_to_status_str.find(code);
    if(it == int_to_status_str.end())
      return false;

    code_str = it->second;
    return true;
  };
};

namespace HttpHeaderField {
  enum Type {
    UNKNOWN_TYPE,
    A_IM,
    ACCEPT,
    ACCEPT_CH,
    ACCEPT_CHARSET,
    ACCEPT_DATETIME,
    ACCEPT_ENCODING,
    ACCEPT_LANGUAGE,
    ACCEPT_PATCH,
    ACCEPT_RANGES,
    ACCESS_CONTROL_ALLOW_CREDENTIALS,
    ACCESS_CONTROL_ALLOW_HEADERS,
    ACCESS_CONTROL_ALLOW_METHODS,
    ACCESS_CONTROL_ALLOW_ORIGIN,
    ACCESS_CONTROL_EXPOSE_HEADERS,
    ACCESS_CONTROL_MAX_AGE,
    ACCESS_CONTROL_REQUEST_HEADERS,
    ACCESS_CONTROL_REQUEST_METHOD,
    AGE,
    ALLOW,
    ALT_SVC,
    AUTHORIZATION,
    CACHE_CONTROL,
    CONNECTION,
    CONTENT_DISPOSITION,
    CONTENT_ENCODING,
    CONTENT_LANGUAGE,
    CONTENT_LENGTH,
    CONTENT_LOCATION,
    CONTENT_MD5,
    CONTENT_RANGE,
    CONTENT_SECURITY_POLICY,
    CONTENT_TYPE,
    COOKIE,
    DATE,
    DELTA_BASE,
    DNT,
    ETAG,
    EXPECT,
    EXPECT_CT,
    EXPIRES,
    FORWARDED,
    FROM,
    FRONT_END_HTTPS,
    HOST,
    HTTP2_SETTINGS,
    IF_MATCH,
    IF_MODIFIED_SINCE,
    IF_NONE_MATCH,
    IF_RANGE,
    IF_UNMODIFIED_SINCE,
    IM,
    LAST_MODIFIED,
    LINK,
    LOCATION,
    MAX_FORWARDS,
    NEL,
    ORIGIN,
    P3P,
    PERMISSIONS_POLICY,
    PRAGMA,
    PREFER,
    PREFERENCE_APPLIED,
    PROXY_AUTHENTICATE,
    PROXY_AUTHORIZATION,
    PROXY_CONNECTION,
    PUBLIC_KEY_PINS,
    RANGE,
    REFERER,
    REFRESH,
    REPORT_TO,
    RETRY_AFTER,
    SAVE_DATA,
    SEC_CH_UA_ARCH,
    SEC_CH_UA_BITNESS,
    SEC_CH_UA_FULL_VERSION_LIST,
    SEC_CH_UA_FULL_VERSION,
    SEC_CH_UA_MOBILE,
    SEC_CH_UA_MODEL,
    SEC_CH_UA_PLATFORM_VERSION,
    SEC_CH_UA_PLATFORM,
    SEC_CH_UA,
    SEC_FETCH_DEST,
    SEC_FETCH_MODE,
    SEC_FETCH_SITE,
    SEC_FETCH_USER,
    SEC_GPC,
    SEC_WEBSOCKET_ACCEPT,
    SEC_WEBSOCKET_EXTENSIONS,
    SEC_WEBSOCKET_KEY,
    SEC_WEBSOCKET_PROTOCOL,
    SEC_WEBSOCKET_VERSION,
    SERVER,
    SET_COOKIE,
    STATUS,
    STRICT_TRANSPORT_SECURITY,
    TE,
    TIMING_ALLOW_ORIGIN,
    TK,
    TRAILER,
    TRANSFER_ENCODING,
    UPGRADE,
    UPGRADE_INSECURE_REQUESTS,
    USER_AGENT,
    VARY,
    VIA,
    WARNING,
    WWW_AUTHENTICATE,
    X_ATT_DEVICEID,
    X_CONTENT_DURATION,
    X_CONTENT_SECURITY_POLICY,
    X_CONTENT_TYPE_OPTIONS,
    X_CORRELATION_ID,
    X_CSRF_TOKEN,
    X_FORWARDED_FOR,
    X_FORWARDED_HOST,
    X_FORWARDED_PROTO,
    X_FRAME_OPTIONS,
    X_HTTP_METHOD_OVERRIDE,
    X_POWERED_BY,
    X_REDIRECT_BY,
    X_REQUEST_ID,
    X_REQUESTED_WITH,
    X_UA_COMPATIBLE,
    X_UIDH,
    X_WAP_PROFILE,
    X_WEBKIT_CSP,
    X_XSS_PROTECTION
  };

  static const std::map<std::string, Type> lowercase_field_str_to_enum = {
    {"a-im", Type::A_IM},
    {"accept", Type::ACCEPT},
    {"accept-ch", Type::ACCEPT_CH},
    {"accept-charset", Type::ACCEPT_CHARSET},
    {"accept-datetime", Type::ACCEPT_DATETIME},
    {"accept-encoding", Type::ACCEPT_ENCODING},
    {"accept-language", Type::ACCEPT_LANGUAGE},
    {"accept-patch", Type::ACCEPT_PATCH},
    {"accept-ranges", Type::ACCEPT_RANGES},
    {"access-control-allow-credentials", Type::ACCESS_CONTROL_ALLOW_CREDENTIALS},
    {"access-control-allow-headers", Type::ACCESS_CONTROL_ALLOW_HEADERS},
    {"access-control-allow-methods", Type::ACCESS_CONTROL_ALLOW_METHODS},
    {"access-control-allow-origin", Type::ACCESS_CONTROL_ALLOW_ORIGIN},
    {"access-control-expose-headers", Type::ACCESS_CONTROL_EXPOSE_HEADERS},
    {"access-control-max-age", Type::ACCESS_CONTROL_MAX_AGE},
    {"access-control-request-headers", Type::ACCESS_CONTROL_REQUEST_HEADERS},
    {"access-control-request-method", Type::ACCESS_CONTROL_REQUEST_METHOD},
    {"age", Type::AGE},
    {"allow", Type::ALLOW},
    {"alt-svc", Type::ALT_SVC},
    {"authorization", Type::AUTHORIZATION},
    {"cache-control", Type::CACHE_CONTROL},
    {"connection", Type::CONNECTION},
    {"content-disposition", Type::CONTENT_DISPOSITION},
    {"content-encoding", Type::CONTENT_ENCODING},
    {"content-language", Type::CONTENT_LANGUAGE},
    {"content-length", Type::CONTENT_LENGTH},
    {"content-location", Type::CONTENT_LOCATION},
    {"content-md5", Type::CONTENT_MD5},
    {"content-range", Type::CONTENT_RANGE},
    {"content-security-policy", Type::CONTENT_SECURITY_POLICY},
    {"content-type", Type::CONTENT_TYPE},
    {"cookie", Type::COOKIE},
    {"date", Type::DATE},
    {"delta-base", Type::DELTA_BASE},
    {"dnt", Type::DNT},
    {"etag", Type::ETAG},
    {"expect", Type::EXPECT},
    {"expect-ct", Type::EXPECT_CT},
    {"expires", Type::EXPIRES},
    {"forwarded", Type::FORWARDED},
    {"from", Type::FROM},
    {"front-end-https", Type::FRONT_END_HTTPS},
    {"host", Type::HOST},
    {"http2-settings", Type::HTTP2_SETTINGS},
    {"if-match", Type::IF_MATCH},
    {"if-modified-since", Type::IF_MODIFIED_SINCE},
    {"if-none-match", Type::IF_NONE_MATCH},
    {"if-range", Type::IF_RANGE},
    {"if-unmodified-since", Type::IF_UNMODIFIED_SINCE},
    {"im", Type::IM},
    {"last-modified", Type::LAST_MODIFIED},
    {"link", Type::LINK},
    {"location", Type::LOCATION},
    {"max-forwards", Type::MAX_FORWARDS},
    {"nel", Type::NEL},
    {"origin", Type::ORIGIN},
    {"p3p", Type::P3P},
    {"permissions-policy", Type::PERMISSIONS_POLICY},
    {"pragma", Type::PRAGMA},
    {"prefer", Type::PREFER},
    {"preference-applied", Type::PREFERENCE_APPLIED},
    {"proxy-authenticate", Type::PROXY_AUTHENTICATE},
    {"proxy-authorization", Type::PROXY_AUTHORIZATION},
    {"proxy-connection", Type::PROXY_CONNECTION},
    {"public-key-pins", Type::PUBLIC_KEY_PINS},
    {"range", Type::RANGE},
    {"referer", Type::REFERER},
    {"refresh", Type::REFRESH},
    {"report-to", Type::REPORT_TO},
    {"retry-after", Type::RETRY_AFTER},
    {"save-data", Type::SAVE_DATA},
    {"sec-ch-ua-arch", Type::SEC_CH_UA_ARCH},
    {"sec-ch-ua-bitness", Type::SEC_CH_UA_BITNESS},
    {"sec-ch-ua-full-version-list", Type::SEC_CH_UA_FULL_VERSION_LIST},
    {"sec-ch-ua-full-version", Type::SEC_CH_UA_FULL_VERSION},
    {"sec-ch-ua-mobile", Type::SEC_CH_UA_MOBILE},
    {"sec-ch-ua-model", Type::SEC_CH_UA_MODEL},
    {"sec-ch-ua-platform-version", Type::SEC_CH_UA_PLATFORM_VERSION},
    {"sec-ch-ua-platform", Type::SEC_CH_UA_PLATFORM},
    {"sec-ch-ua", Type::SEC_CH_UA},
    {"sec-fetch-dest", Type::SEC_FETCH_DEST},
    {"sec-fetch-mode", Type::SEC_FETCH_MODE},
    {"sec-fetch-site", Type::SEC_FETCH_SITE},
    {"sec-fetch-user", Type::SEC_FETCH_USER},
    {"sec-gpc", Type::SEC_GPC},
    {"sec-websocket-accept", Type::SEC_WEBSOCKET_ACCEPT},
    {"sec-websocket-extensions", Type::SEC_WEBSOCKET_EXTENSIONS},
    {"sec-websocket-key", Type::SEC_WEBSOCKET_KEY},
    {"sec-websocket-protocol", Type::SEC_WEBSOCKET_PROTOCOL},
    {"sec-websocket-version", Type::SEC_WEBSOCKET_VERSION},
    {"server", Type::SERVER},
    {"set-cookie", Type::SET_COOKIE},
    {"status", Type::STATUS},
    {"strict-transport-security", Type::STRICT_TRANSPORT_SECURITY},
    {"te", Type::TE},
    {"timing-allow-origin", Type::TIMING_ALLOW_ORIGIN},
    {"tk", Type::TK},
    {"trailer", Type::TRAILER},
    {"transfer-encoding", Type::TRANSFER_ENCODING},
    {"upgrade", Type::UPGRADE},
    {"upgrade-insecure-requests", Type::UPGRADE_INSECURE_REQUESTS},
    {"user-agent", Type::USER_AGENT},
    {"vary", Type::VARY},
    {"via", Type::VIA},
    {"warning", Type::WARNING},
    {"www-authenticate", Type::WWW_AUTHENTICATE},
    {"x-att-deviceid", Type::X_ATT_DEVICEID},
    {"x-content-duration", Type::X_CONTENT_DURATION},
    {"x-content-security-policy", Type::X_CONTENT_SECURITY_POLICY},
    {"x-content-type-options", Type::X_CONTENT_TYPE_OPTIONS},
    {"x-correlation-id", Type::X_CORRELATION_ID},
    {"x-csrf-token", Type::X_CSRF_TOKEN},
    {"x-forwarded-for", Type::X_FORWARDED_FOR},
    {"x-forwarded-host", Type::X_FORWARDED_HOST},
    {"x-forwarded-proto", Type::X_FORWARDED_PROTO},
    {"x-frame-options", Type::X_FRAME_OPTIONS},
    {"x-http-method-override", Type::X_HTTP_METHOD_OVERRIDE},
    {"x-powered-by", Type::X_POWERED_BY},
    {"x-redirect-by", Type::X_REDIRECT_BY},
    {"x-request-id", Type::X_REQUEST_ID},
    {"x-requested-with", Type::X_REQUESTED_WITH},
    {"x-ua-compatible", Type::X_UA_COMPATIBLE},
    {"x-uidh", Type::X_UIDH},
    {"x-wap-profile", Type::X_WAP_PROFILE},
    {"x-webkit-csp", Type::X_WEBKIT_CSP},
    {"x-xss-protection", Type::X_XSS_PROTECTION}
  };

  static const std::map<Type, std::string> enum_to_field_str = {
    {Type::A_IM, "A-IM"},
    {Type::ACCEPT, "Accept"},
    {Type::ACCEPT_CH, "Accept-CH"},
    {Type::ACCEPT_CHARSET, "Accept-Charset"},
    {Type::ACCEPT_DATETIME, "Accept-Datetime"},
    {Type::ACCEPT_ENCODING, "Accept-Encoding"},
    {Type::ACCEPT_LANGUAGE, "Accept-Language"},
    {Type::ACCEPT_PATCH, "Accept-Patch"},
    {Type::ACCEPT_RANGES, "Accept-Ranges"},
    {Type::ACCESS_CONTROL_ALLOW_CREDENTIALS, "Access-Control-Allow-Credentials"},
    {Type::ACCESS_CONTROL_ALLOW_HEADERS, "Access-Control-Allow-Headers"},
    {Type::ACCESS_CONTROL_ALLOW_METHODS, "Access-Control-Allow-Methods"},
    {Type::ACCESS_CONTROL_ALLOW_ORIGIN, "Access-Control-Allow-Origin"},
    {Type::ACCESS_CONTROL_EXPOSE_HEADERS, "Access-Control-Expose-Headers"},
    {Type::ACCESS_CONTROL_MAX_AGE, "Access-Control-Max-Age"},
    {Type::ACCESS_CONTROL_REQUEST_HEADERS, "Access-Control-Request-Headers"},
    {Type::ACCESS_CONTROL_REQUEST_METHOD, "Access-Control-Request-Method"},
    {Type::AGE, "Age"},
    {Type::ALLOW, "Allow"},
    {Type::ALT_SVC, "Alt-Svc"},
    {Type::AUTHORIZATION, "Authorization"},
    {Type::CACHE_CONTROL, "Cache-Control"},
    {Type::CONNECTION, "Connection"},
    {Type::CONTENT_DISPOSITION, "Content-Disposition"},
    {Type::CONTENT_ENCODING, "Content-Encoding"},
    {Type::CONTENT_LANGUAGE, "Content-Language"},
    {Type::CONTENT_LENGTH, "Content-Length"},
    {Type::CONTENT_LOCATION, "Content-Location"},
    {Type::CONTENT_MD5, "Content-MD5"},
    {Type::CONTENT_RANGE, "Content-Range"},
    {Type::CONTENT_SECURITY_POLICY, "Content-Security-Policy"},
    {Type::CONTENT_TYPE, "Content-Type"},
    {Type::COOKIE, "Cookie"},
    {Type::DATE, "Date"},
    {Type::DELTA_BASE, "Delta-Base"},
    {Type::DNT, "DNT"},
    {Type::ETAG, "ETag"},
    {Type::EXPECT, "Expect"},
    {Type::EXPECT_CT, "Expect-CT"},
    {Type::EXPIRES, "Expires"},
    {Type::FORWARDED, "Forwarded"},
    {Type::FROM, "From"},
    {Type::FRONT_END_HTTPS, "Front-End-Https"},
    {Type::HOST, "Host"},
    {Type::HTTP2_SETTINGS, "HTTP2-Settings"},
    {Type::IF_MATCH, "If-Match"},
    {Type::IF_MODIFIED_SINCE, "If-Modified-Since"},
    {Type::IF_NONE_MATCH, "If-None-Match"},
    {Type::IF_RANGE, "If-Range"},
    {Type::IF_UNMODIFIED_SINCE, "If-Unmodified-Since"},
    {Type::IM, "IM"},
    {Type::LAST_MODIFIED, "Last-Modified"},
    {Type::LINK, "Link"},
    {Type::LOCATION, "Location"},
    {Type::MAX_FORWARDS, "Max-Forwards"},
    {Type::NEL, "NEL"},
    {Type::ORIGIN, "Origin"},
    {Type::P3P, "P3P"},
    {Type::PERMISSIONS_POLICY, "Permissions-Policy"},
    {Type::PRAGMA, "Pragma"},
    {Type::PREFER, "Prefer"},
    {Type::PREFERENCE_APPLIED, "Preference-Applied"},
    {Type::PROXY_AUTHENTICATE, "Proxy-Authenticate"},
    {Type::PROXY_AUTHORIZATION, "Proxy-Authorization"},
    {Type::PROXY_CONNECTION, "Proxy-Connection"},
    {Type::PUBLIC_KEY_PINS, "Public-Key-Pins"},
    {Type::RANGE, "Range"},
    {Type::REFERER, "Referer"},
    {Type::REFRESH, "Refresh"},
    {Type::REPORT_TO, "Report-To"},
    {Type::RETRY_AFTER, "Retry-After"},
    {Type::SAVE_DATA, "Save-Data"},
    {Type::SEC_CH_UA_ARCH, "Sec-CH-UA-Arch"},
    {Type::SEC_CH_UA_BITNESS, "Sec-CH-UA-Bitness"},
    {Type::SEC_CH_UA_FULL_VERSION_LIST, "Sec-CH-UA-Full-Version-List"},
    {Type::SEC_CH_UA_FULL_VERSION, "Sec-CH-UA-Full-Version"},
    {Type::SEC_CH_UA_MOBILE, "Sec-CH-UA-Mobile"},
    {Type::SEC_CH_UA_MODEL, "Sec-CH-UA-Model"},
    {Type::SEC_CH_UA_PLATFORM_VERSION, "Sec-CH-UA-Platform-Version"},
    {Type::SEC_CH_UA_PLATFORM, "Sec-CH-UA-Platform"},
    {Type::SEC_CH_UA, "Sec-CH-UA"},
    {Type::SERVER, "Server"},
    {Type::SEC_FETCH_DEST, "Sec-Fetch-Dest"},
    {Type::SEC_FETCH_MODE, "Sec-Fetch-Mode"},
    {Type::SEC_FETCH_SITE, "Sec-Fetch-Site"},
    {Type::SEC_FETCH_USER, "Sec-Fetch-User"},
    {Type::SEC_GPC, "Sec-GPC"},
    {Type::SEC_WEBSOCKET_ACCEPT, "Sec-WebSocket-Accept"},
    {Type::SEC_WEBSOCKET_EXTENSIONS, "Sec-WebSocket-Extensions"},
    {Type::SEC_WEBSOCKET_KEY, "Sec-WebSocket-Key"},
    {Type::SEC_WEBSOCKET_PROTOCOL, "Sec-WebSocket-Protocol"},
    {Type::SEC_WEBSOCKET_VERSION, "Sec-WebSocket-Version"},
    {Type::SET_COOKIE, "Set-Cookie"},
    {Type::STATUS, "Status"},
    {Type::STRICT_TRANSPORT_SECURITY, "Strict-Transport-Security"},
    {Type::TE, "TE"},
    {Type::TIMING_ALLOW_ORIGIN, "Timing-Allow-Origin"},
    {Type::TK, "Tk"},
    {Type::TRAILER, "Trailer"},
    {Type::TRANSFER_ENCODING, "Transfer-Encoding"},
    {Type::UPGRADE, "Upgrade"},
    {Type::UPGRADE_INSECURE_REQUESTS, "Upgrade-Insecure-Requests"},
    {Type::USER_AGENT, "User-Agent"},
    {Type::VARY, "Vary"},
    {Type::VIA, "Via"},
    {Type::WARNING, "Warning"},
    {Type::WWW_AUTHENTICATE, "WWW-Authenticate"},
    {Type::X_ATT_DEVICEID, "X-ATT-DeviceId"},
    {Type::X_CONTENT_DURATION, "X-Content-Duration"},
    {Type::X_CONTENT_SECURITY_POLICY, "X-Content-Security-Policy"},
    {Type::X_CONTENT_TYPE_OPTIONS, "X-Content-Type-Options"},
    {Type::X_CORRELATION_ID, "X-Correlation-ID"},
    {Type::X_CSRF_TOKEN, "X-Csrf-Token"},
    {Type::X_FORWARDED_FOR, "X-Forwarded-For"},
    {Type::X_FORWARDED_HOST, "X-Forwarded-Host"},
    {Type::X_FORWARDED_PROTO, "X-Forwarded-Proto"},
    {Type::X_FRAME_OPTIONS, "X-Frame-Options"},
    {Type::X_HTTP_METHOD_OVERRIDE, "X-Http-Method-Override"},
    {Type::X_POWERED_BY, "X-Powered-By"},
    {Type::X_REDIRECT_BY, "X-Redirect-By"},
    {Type::X_REQUEST_ID, "X-Request-ID"},
    {Type::X_REQUESTED_WITH, "X-Requested-With"},
    {Type::X_UA_COMPATIBLE, "X-UA-Compatible"},
    {Type::X_UIDH, "X-UIDH"},
    {Type::X_WAP_PROFILE, "X-Wap-Profile"},
    {Type::X_WEBKIT_CSP, "X-WebKit-CSP"},
    {Type::X_XSS_PROTECTION, "X-XSS-Protection"}
  };

  static bool GetTypeFromString(std::string type_str, Type& type) {
    auto it = lowercase_field_str_to_enum.find(StringUtils::Lowercase(type_str));
    if(it == lowercase_field_str_to_enum.end())
      return false;

    type = it->second;
    return true;
  };

  static std::string GetStringFromType(Type type) {
    auto it = enum_to_field_str.find(type);
    return it->second;
  };
};
