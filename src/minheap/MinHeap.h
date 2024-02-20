#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

class MinHeap
{

public:

  MinHeap() = default;

  bool isEmpty() const
  {
    return m_size == 0;
  }

  int top() const
  {
    return m_vector[1];
  }

  void push(int value);

  void pop();

  void erase(int val);

private:

  void m_shiftUp(std::size_t i);

  void m_shiftDown(std::size_t i);

  static constexpr std::size_t m_getParent(std::size_t i)
  {
    return i >> 1;
  }

  static constexpr std::size_t m_getLeft(std::size_t i)
  {
    return i << 1;
  }

  static constexpr std::size_t m_getRight(std::size_t i)
  {
    return m_getLeft(i) + 1;
  }

private:

  std::vector<int> m_vector = { 0 }; // first garbage value becouse we need to be able to bit shift
  std::unordered_map<int, std::size_t> entries;

  std::size_t m_size = 0;

};
