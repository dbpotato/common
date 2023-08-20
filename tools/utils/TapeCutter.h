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

#pragma once

#include "Data.h"
#include <vector>

class DataResource;

class TapeCutter {
public:
  TapeCutter();
  virtual bool AddData(std::shared_ptr<Data> data);
protected:
  virtual bool FindCutHeader(std::shared_ptr<Data> data, uint32_t& out_expected_cut_size) = 0;
  virtual void FindCutFooter(std::shared_ptr<Data> data) = 0;
  virtual uint32_t AddDataToCurrentCut(std::shared_ptr<Data> data) = 0;

  std::shared_ptr<Data> _unfinished_header;
  uint32_t _expected_cut_size;
  uint32_t _current_cut_size;

private:
  void Reset();
  void OnEndFound(std::shared_ptr<Data> data);
  bool _header_found;
};
