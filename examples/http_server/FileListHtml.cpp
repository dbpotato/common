/*
Copyright (c) 2024 Adam Kaniewski

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

#include "FileListHtml.h"
#include "StringUtils.h"

#include <algorithm>
#include <sstream>


const static std::string HEADER = R"""(
<!DOCTYPE html>
  <head>
  <style>
    .icon {
      background-size: 100% 100% ;
      border: 0px;
      width: 16px;
      height: 16px;
      margin-right: 5px;
    }
    .dir {
      background-image: url(data:image/jpeg;base64,/9j/4AAQSkZJRgABAQIAHAAcAAD/2wBDABALDA4MChAODQ4SERATGCgaGBYWGDEjJR0oOjM9PDkzODdASFxOQERXRTc4UG1RV19iZ2hnPk1xeXBkeFxlZ2P/2wBDARESEhgVGC8aGi9jQjhCY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2P/wgARCABAAEADARIAAhEBAxEB/8QAGQABAQEBAQEAAAAAAAAAAAAAAQIAAwQF/8QAGAEAAwEBAAAAAAAAAAAAAAAAAAECAwT/2gAMAwEAAhADEAAAAftTyaaqI7NG0SNIDxivR5g082neenNnaCAvI7efG3ljdxNVMPtHVlUtBjbOV4TwhAvXOmWg2QQMJ7ALP//EAB4QAAICAgMBAQAAAAAAAAAAAAABAhEDIRASMSAw/9oACAEBAAEFArLLFItHZHZcx2qRUSonVE0lHH6Y/JLdFcZNxxWmzH5JW6KQ3yyHklbqPyyOlf7PT+km3//EAB0RAAICAgMBAAAAAAAAAAAAAAABAhEQIRIgMTD/2gAIAQMBAT8BoejkjRo0JJko1ifvRQYoUSV4mrZwFBdffh//xAAiEQADAAEDAwUAAAAAAAAAAAAAAQIDERIgECJBEyExMlH/2gAIAQIBAT8BdEy6PTr9ErR3neVVStTHk3jZh+vVtL5HnXgrNuWhF7WMxUpn3HnXgeWma8HquaTZ/8QAHhAAAAYCAwAAAAAAAAAAAAAAAAECIDEyEHEhQIH/2gAIAQEABj8CZIkS+Gwn0VQKozzmqT2KIFSdVJ7FS7v/xAAiEAACAgAGAwEBAAAAAAAAAAAAAREhECAxQWFxUeHxofD/2gAIAQEAAT8hbSS8kvIlq6ZxDgHCE09GMRMlbnGjqOJYKZJDyw2avZJqh/L4Ky/L4NllLUVmEUahr9lx3G9E/d6IUnC+ES4J2ah794QEXl4Q3OO41s3NWOWXcVXuTnVwPOkJI//aAAwDAQACAAMAAAAQHJTd5OKukboiXe7zN6YerP/EABwRAQEBAQEAAwEAAAAAAAAAAAEAESExIEGBYf/aAAgBAwEBPxAEhY38JXfu/c/he+DnwgXyV7bdGcwck4w/uBBllk+Rg2wuXLly5KB1v//EAB4RAAMAAgIDAQAAAAAAAAAAAAABESFREDEggaFB/9oACAECAQE/EHN5YmqfEQRtM9Pp6fS0QuaaMj8DtQr0o6hC7QbJkKCnQ/cg2fZeE8iNkM6I9EeiPRHoj0NIkf/EACEQAQADAAEEAwEBAAAAAAAAAAEAESExUXGx8UFhkYGh/9oACAEBAAE/EGLUj1ErbtNph7ye0nsIbYJ9Rax8sSkT5D+z6P1PcR6SMDDZKxeVc2Yr7vwRrOHyt8MfditqKeVp2gSXJ3YyEGjGxa7z9H4I1LH8B8pRPHkPcMfpqNgwixAo0jFrvP2PgmhweeXiVhe3aP8AIitYsWDjvKO5Gpu1g+WPjgixYsWDiUSy05fwRs6y5cWLFg6RQGk2KzY3EekR6RkFr+T/2Q==);
    }
    .file {
      background-image: url(data:image/jpeg;base64,/9j/4AAQSkZJRgABAQIAdgB2AAD/2wBDABALDA4MChAODQ4SERATGCgaGBYWGDEjJR0oOjM9PDkzODdASFxOQERXRTc4UG1RV19iZ2hnPk1xeXBkeFxlZ2P/2wBDARESEhgVGC8aGi9jQjhCY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2P/wgARCABAAEADASIAAhEBAxEB/8QAGAAAAwEBAAAAAAAAAAAAAAAAAQIDAAb/xAAXAQADAQAAAAAAAAAAAAAAAAAAAQID/9oADAMBAAIQAxAAAAHoBlAIjRRAEM1RrlpVAQxGVgEJ0pOm+bbAEG2dBWUHdG0lp1REXbSxOgpB51Z//8QAHhAAAgICAwEBAAAAAAAAAAAAAAECMSEiAxESECD/2gAIAQEAAQUC+SeNzc2NiL+NjZ3M7mdzFUKlTidL8KoU6lWDtGDAqhTH2bm5ubiqFSJZXlM8Hg8CwoEidcd+RrHkpcZIyZMmTJkgj//EABkRAQADAQEAAAAAAAAAAAAAAAEAEBEgIf/aAAgBAwEBPwGFnPk0ois1r//EABsRAAMAAwEBAAAAAAAAAAAAAAABEQIQIXHw/9oACAECAQE/ATIvohnDn1EMjJlrJXgsElCJ6//EAB0QAAEEAwEBAAAAAAAAAAAAAAABMTJBEBEgITD/2gAIAQEABj8CzRRRXLDET3LkiZMmPvLbIkSBDjwoooo94cdR1HXp1NbJL8//xAAhEAACAgEEAgMAAAAAAAAAAAAAARExIRBRYXEggUGR4f/aAAgBAQABPyHSBoE2VHoep6mZakyYoqnFHtBm8EMo6JUFxl1pYMGDX9hR1pOrBO6ckncJ3iuFHBR0VMW7wAZS5NCgiyOBUHZEIhEEwFSgreIjy+B5lIsnZP8AQVEyU9CSieBPAngeg+h6Dkj/2gAMAwEAAgADAAAAEGKCg23aKYMlZs+8dv/EAB4RAAMAAQQDAAAAAAAAAAAAAAABEUEhMWHwcZHx/9oACAEDAQE/EBNCPgbpga9gr2DyLBU+Hg9DyNI0NOiRsx8n/8QAIREAAgEDAwUAAAAAAAAAAAAAAAERITFBEGGRcYHh8PH/2gAIAQIBAT8QGh1nsQ+PAkO4qacnq+hh0MiWqnk2HyLEiEbZFVqjvkK6g//EACUQAQACAAUEAgMBAAAAAAAAAAEAESExQVHhEHGRsYHwYcHxof/aAAgBAQABPxDpbMwX3gRw3vF/ri85i85jGuFl1t0II9DFrCUO1vzHkuZyjmBENUOkIK9OkqVy81gWX35j+T78x/J55iN/nmD6B6ZnfHuAFgvIjx0eGjxkeIiGxO7Sf5kz/j3MJhxaxgwekcFh31MrtP0+5ijI3Y1DrrNmPJR5KPJQSKtaseHsT9fuH6rTaEG0JSsdYymq3IYtApocWMkaEqauc9KJg+Z9rPtY/wBMXZ5xbPONtPKI72rvP//Z);
    }
    .line {
      display: flex;
      align-items: center;
      margin-bottom: 5px;
    }
  </style>
  </head>
  <body>
)""";

const static std::string FOOTER = R"""(
  </body>
</html>
)""";

FileListHtml::FileListHtml(const std::filesystem::path& html_dir)
    : _html_dir(html_dir) {
}

void FileListHtml::CreateFileList(const std::filesystem::path& path, std::vector<std::filesystem::path>& output) {
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator{path}) {
    if (std::filesystem::is_directory(entry)) {
      output.emplace_back(entry.path());
    } else {
      files.emplace_back(entry.path());
    }
  }
  std::sort(output.begin(), output.end());
  std::sort(files.begin(), files.end());
  output.insert(output.end(), files.begin(), files.end());
}

std::string FileListHtml::CreateLinkElement(const std::filesystem::path& path) {
  return CreateLinkElement(path, path.filename().string());
}

std::string FileListHtml::CreateLinkElement(const std::filesystem::path& path, const std::string& name) {
  std::stringstream sstream;
  sstream << "<div class='line'>";
  if(std::filesystem::is_directory(path) || !name.compare("..")) {
    sstream << "<div class='icon dir'>";
  } else {
    sstream << "<div class='icon file'>";
  }
  sstream << "</div><a href=\"/"
          << std::filesystem::relative(path, _html_dir).string()
          << "\">"
          << name
          << "</a></div>\n";
  return sstream.str();
}

std::string FileListHtml::CreateHtmlList(const std::filesystem::path& path) {
  std::vector<std::filesystem::path> file_list;
  CreateFileList(path, file_list);

  std::stringstream sstream;
  sstream << HEADER;
  if(std::filesystem::canonical(path) != _html_dir) {
    sstream << CreateLinkElement(path.parent_path(), "..");
  }
  for(const auto& entry : file_list) {
    sstream << CreateLinkElement(entry);
  }
  sstream << FOOTER;
  return sstream.str();
}
