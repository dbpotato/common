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

#include "TapeCutter.h"

const int MAX_HEADER_LENGTH = 8*1024;

TapeCutter::TapeCutter()
    : _expected_cut_size(0)
    , _current_cut_size(0)
    , _header_found(false) {
  _unfinished_header = std::make_shared<Data>();
}

bool TapeCutter::AddData(std::shared_ptr<Data> data) {
  bool repeat = false;
  uint32_t available_cut_data = 0;

  do {
    repeat = false;
    if(!_header_found) {
      _unfinished_header->Add(data);
      _header_found = FindCutHeader(_unfinished_header, _expected_cut_size);
      if(!_header_found) {
        if(_unfinished_header->GetCurrentSize() > MAX_HEADER_LENGTH) {
          Reset();
          return false;
        } else {
          data->Swap(_unfinished_header);
          return true;
        }
      } else {
        data->Swap(_unfinished_header);
      }
    }

    if(!_expected_cut_size) {
      OnEndFound(data);
      if(!data->GetCurrentSize()) {
        return true;
      }
      repeat = true;
      continue;
    }

    if(_current_cut_size + data->GetCurrentSize() > _expected_cut_size) {
      repeat = true;
      available_cut_data = _expected_cut_size - _current_cut_size;
    } else{
      available_cut_data = data->GetCurrentSize();
    }

    std::shared_ptr<Data> add_data = Data::MakeShallowCopy(data);
    add_data->SetCurrentSize(available_cut_data);

    _current_cut_size = AddDataToCurrentCut(add_data);

    if(_current_cut_size == _expected_cut_size) {
      data->AddOffset(available_cut_data);
      OnEndFound(data);
      repeat = true;
    }

    if(repeat) {
      if(!data->GetCurrentSize()) {
        return true;
      }
    }
  } while(repeat);
  return true;
}

void TapeCutter::Reset() {
  _unfinished_header = std::make_shared<Data>();
  _expected_cut_size = 0;
  _current_cut_size = 0;
  _header_found = false;
}

void TapeCutter::OnEndFound(std::shared_ptr<Data> data) {
  Reset();
  FindCutFooter(data);
}