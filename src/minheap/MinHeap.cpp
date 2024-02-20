#include "MinHeap.h"
#include <utility>

void MinHeap::push(int value)
{

  if (m_size + 1 >= m_vector.size())
  {

    m_vector.push_back({});

  }

  m_vector[++m_size] = value;
  m_shiftUp(m_size);

}

void MinHeap::pop()
{

  std::swap(m_vector[1], m_vector[m_size--]);
  m_shiftDown(1);

}

void MinHeap::m_shiftUp(std::size_t i) // if future we will need to remove recursion
{

  if (i > m_size) return;
  if (i == 1) return;

  if (m_vector[i] < m_vector[m_getParent(i)])
  {

    std::swap(m_vector[m_getParent(i)], m_vector[i]);

  }

  m_shiftUp(m_getParent(i));

}

void MinHeap::m_shiftDown(std::size_t i) // implement this using while loop
{

  if (i > m_size) return;

  std::size_t swapId = i;

  if (m_getLeft(i) <= m_size && m_vector[i] > m_vector[m_getLeft(i)]) swapId = m_getLeft(i);
  if (m_getRight(i) <= m_size && m_vector[swapId] > m_vector[m_getRight(i)]) swapId = m_getRight(i);

  if (swapId == i) return;

  std::swap(m_vector[i], m_vector[swapId]);

  m_shiftDown(swapId);

}
