/*
Copyright (c) 2019 Adam Kaniewski

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

#include <vector>
#include <atomic>
#include <algorithm>

template<class T>
class Collector {
private:
  class Stack {
  private:
    struct Node {
      T _data;
      Node* _next;
      Node(const T& data) : _data(data), _next(nullptr) {}
    };
    std::atomic<Node*> _head;
  public:
    Stack() : _head(nullptr) {}
    ~Stack();
    void Insert(const T& data);
    void Clear();
    void MoveToVector(std::vector<T>& out);
  };

  Stack _stack;
  Stack _stack_swap;
  std::atomic<Stack*> _current_stack;
  bool _swap;

public:
  Collector();
  ~Collector();
  bool Add(const T& data);
  void Collect(std::vector<T>& out_vec);
};

template <class T>
Collector<T>::Stack::~Stack() {
  Clear();
}

template <class T>
void Collector<T>::Stack::Insert(const T& data) {
  Node* new_node = new Node(data);
  new_node->_next = _head.load(std::memory_order_relaxed);

  while(!_head.compare_exchange_weak(new_node->_next,
                                     new_node,
                                     std::memory_order_release,
                                     std::memory_order_relaxed));
}

template <class T>
void Collector<T>::Stack::MoveToVector(std::vector<T>& out_vec) {
  Node* node = _head.load();
  while(node) {
    out_vec.push_back(node->_data);
    Node* next = node->_next;
    delete node;
    node = next;
  }

  _head = nullptr;
  std::reverse(std::begin(out_vec), std::end(out_vec));
}

template <class T>
void Collector<T>::Stack::Clear() {
  Node* node = _head.load();
  while(node) {
    Node* next = node->_next;
    delete node;
    node = next;
  }

  _head = nullptr;
}

template <class T>
Collector<T>::Collector()
    : _current_stack(&_stack)
    , _swap(false) {
}

template <class T>
Collector<T>::~Collector() {
  _current_stack.store(nullptr, std::memory_order_release);
}

template <class T>
bool Collector<T>::Add(const T& data) {
  Stack* stack = _current_stack.load(std::memory_order_acquire);
  if(!stack)
    return false;

  stack->Insert(data);
  return true;
}

template <class T>
void Collector<T>::Collect(std::vector<T>& out_vec) {
  Stack* read_stack = nullptr;
  if(!_swap) {
    read_stack = &_stack;
    _current_stack.store(&_stack_swap, std::memory_order_release);
  }
  else {
    read_stack = &_stack_swap;
    _current_stack.store(&_stack, std::memory_order_release);
  }
  read_stack->MoveToVector(out_vec);
  _swap = !_swap;
}
