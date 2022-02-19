 /*
Copyright (c) 2021 Adam Kaniewski

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

#include "MimeTypeFinder.h"
#include "StringUtils.h"

#include <map>
#include <vector>


const std::string DEFUALT_MIME_TYPE = "application/octet-stream";
const std::map<const std::string, const std::string> MIME_TYPE_MAP = {
  {"aac" , "audio/aac"},
  {"abw" , "application/x-abiword"},
  {"arc" , "application/x-freearc"},
  {"avi" , "video/x-msvideo"},
  {"azw" , "application/vnd.amazon.ebook"},
  {"bin" , "application/octet-stream"},
  {"bmp" , "image/bmp"},
  {"bz" , "application/x-bzip"},
  {"bz2" , "application/x-bzip2"},
  {"cda" , "application/x-cdf"},
  {"csh" , "application/x-csh"},
  {"css" , "text/css"},
  {"csv" , "text/csv"},
  {"doc" , "application/msword"},
  {"docx" , "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
  {"eot" , "application/vnd.ms-fontobject"},
  {"epub" , "application/epub+zip"},
  {"gz" , "application/gzip"},
  {"gif" , "image/gif"},
  {"htm" , "text/html"},
  {"html" , "text/html"},
  {"ico" , "image/vnd.microsoft.icon"},
  {"ics" , "text/calendar"},
  {"jar" , "application/java-archive"},
  {"jpeg" , "image/jpeg"},
  {"jpg" , "image/jpeg"},
  {"js" , "text/javascript"},
  {"json" , "application/json"},
  {"jsonld" , "application/ld+json"},
  {"mid" , "audio/midi"},
  {"midi" , "audio/midi"},
  {"mjs" , "text/javascript"},
  {"mp3" , "audio/mpeg"},
  {"mp4" , "video/mp4"},
  {"mpeg" , "video/mpeg"},
  {"mpkg" , "application/vnd.apple.installer+xml"},
  {"odp" , "application/vnd.oasis.opendocument.presentation"},
  {"ods" , "application/vnd.oasis.opendocument.spreadsheet"},
  {"odt" , "application/vnd.oasis.opendocument.text"},
  {"oga" , "audio/ogg"},
  {"ogv" , "video/ogg"},
  {"ogx" , "application/ogg"},
  {"opus" , "audio/opus"},
  {"otf" , "font/otf"},
  {"png" , "image/png"},
  {"pdf" , "application/pdf"},
  {"php" , "application/x-httpd-php"},
  {"ppt" , "application/vnd.ms-powerpoint"},
  {"pptx" , "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
  {"rar" , "application/vnd.rar"},
  {"rtf" , "application/rtf"},
  {"sh" , "application/x-sh"},
  {"svg" , "image/svg+xml"},
  {"swf" , "application/x-shockwave-flash"},
  {"tar" , "application/x-tar"},
  {"tif" , "image/tiff"},
  {"tiff" , "image/tiff"},
  {"ts" , "video/mp2t"},
  {"ttf" , "font/ttf"},
  {"txt" , "text/plain"},
  {"vsd" , "application/vnd.visio"},
  {"wav" , "audio/wav"},
  {"weba" , "audio/webm"},
  {"webm" , "video/webm"},
  {"webp" , "image/webp"},
  {"woff" , "font/woff"},
  {"woff2" , "font/woff2"},
  {"xhtml" , "application/xhtml+xml"},
  {"xls" , "application/vnd.ms-excel"},
  {"xlsx" , "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
  {"xml" , "application/xml"},
  {"xul" , "application/vnd.mozilla.xul+xml"},
  {"zip" , "application/zip"},
  {"3gp" , "video/3gpp"},
  {"3g2" , "video/3gpp2"},
  {"7z" , "application/x-7z-compressed"}
};

const std::string& MimeTypeFinder::Find(const std::string& resource) {
  std::vector<std::string> split = StringUtils::Split(resource, ".");

  if(!split.size()) {
    return DEFUALT_MIME_TYPE;
  }

  std::string ext = StringUtils::Lowercase(split.back());
  auto it = MIME_TYPE_MAP.find(ext);
  if(it != MIME_TYPE_MAP.end()) {
    return it->second;
  }

  return DEFUALT_MIME_TYPE;
}
