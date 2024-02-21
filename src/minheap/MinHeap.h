#pragma once

#include <functional>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

template <typename T, typename Compare = std::less<T>>
class MinHeap
{

public:

  MinHeap() = default;

  bool empty() const
  {
    return m_size == 0;
  }

  T top() const
  {
    return m_vector[1];
  }

  std::size_t size() const
  {
    return m_size;
  }

  void push(T value)
  {

    if (m_size + 1 >= m_vector.size())
    {

      m_vector.push_back({});

    }

    m_vector[++m_size] = value;
    m_indices[value].insert(m_size);

    m_shiftUp(m_size);

  }

  void pop()
  {

    m_indices.at(m_vector[1]).erase(1);

    if (m_indices.at(m_vector[1]).empty()) m_indices.erase(m_vector[1]);

    if (m_size != 1)
    {
      m_indices.at(m_vector[m_size]).erase(m_size);
      m_indices.at(m_vector[m_size]).insert(1);

    }

    std::swap(m_vector[1], m_vector[m_size--]);
    m_shiftDown(1);

  }

  void erase(T value)
  {

    if (m_indices.find(value) == m_indices.end()) return;

    std::size_t i = *m_indices.at(value).begin();
   
    if (i == 1)
    {

      pop();
      return;

    }

    m_indices.at(value).erase(i);

    if (m_indices.at(value).empty()) m_indices.erase(value);

    if (i != m_size)
    {

      m_indices.at(m_vector[m_size]).erase(m_size);
      m_indices.at(m_vector[m_size]).insert(i);

    }

    std::swap(m_vector[i], m_vector[m_size--]);

    if (m_compare(m_vector[m_getParent(i)], m_vector[i]))
    {

      m_shiftDown(i);
      return;

    }

    m_shiftUp(i);


  }

private:

  void m_shiftUp(std::size_t i)
  {

    if (i > m_size) return;
    if (i == 1) return;

    if (m_compare(m_vector[m_getParent(i)], m_vector[i]))
    {

      // is there no better way to swap it?

      std::size_t a = m_getParent(i);
      std::size_t b = i;

      m_indices.at(m_vector[a]).erase(a);
      m_indices.at(m_vector[a]).insert(b);

      m_indices.at(m_vector[b]).erase(b);
      m_indices.at(m_vector[b]).insert(a);

      std::swap(m_vector[m_getParent(i)], m_vector[i]);

      m_shiftUp(m_getParent(i));

    }

  }

  void m_shiftDown(std::size_t i)
  {

    if (i > m_size) return;

    std::size_t swapId = i;

    if (m_getLeft(i) <= m_size && m_compare(m_vector[i], m_vector[m_getLeft(i)])) swapId = m_getLeft(i);
    if (m_getRight(i) <= m_size && m_compare(m_vector[swapId], m_vector[m_getRight(i)])) swapId = m_getRight(i);

    if (swapId == i) return;

    std::size_t a = i;
    std::size_t b = swapId;

    m_indices.at(m_vector[a]).erase(a);
    m_indices.at(m_vector[a]).insert(b);

    m_indices.at(m_vector[b]).erase(b);
    m_indices.at(m_vector[b]).insert(a);

    std::swap(m_vector[i], m_vector[swapId]);

    m_shiftDown(swapId);

  }

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

  std::vector<T> m_vector = { {} }; // first garbage value becouse we need to be able to bit shift
  std::unordered_map<T, std::unordered_set<std::size_t>> m_indices; // for requests one key value pair would be better

  std::size_t m_size = 0;
  Compare m_compare;

};
