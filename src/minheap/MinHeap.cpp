#include "MinHeap.h"
#include <algorithm>
#include <cstddef>
#include <utility>

void MinHeap::push(int value)
{

  if (m_size + 1 >= m_vector.size())
  {

    m_vector.push_back({});

  }

  m_vector[++m_size] = value;

  m_indices[value].insert(m_size);
  m_shiftUp(m_size);

}

void MinHeap::pop() 
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

void MinHeap::erase(int value)
{

  std::size_t i = *m_indices.at(value).begin();

  std::swap(m_vector[i], m_vector[m_size--]);

  if (i != 1 && m_vector[i] < m_vector[m_getParent(i)]) // i != 1 can be removed cause parent will point to i
  {

    m_shiftUp(i);
    return;

  }

  m_shiftDown(i);

}

void MinHeap::m_shiftUp(std::size_t i) // in future we will need to remove recursion
{

  if (i > m_size) return;
  if (i == 1) return;

  if (m_vector[i] < m_vector[m_getParent(i)])
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

void MinHeap::m_shiftDown(std::size_t i) // implement this using while loop
{

  if (i > m_size) return;

  std::size_t swapId = i;

  if (m_getRight(i) <= m_size && m_vector[i] > m_vector[m_getRight(i)]) swapId = m_getRight(i);
  if (m_getLeft(i) <= m_size && m_vector[swapId] >= m_vector[m_getLeft(i)]) swapId = m_getLeft(i);

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


void MinHeap::m_swap(int& a, int& b)
{

  std::swap(a, b);

}
