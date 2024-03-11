#include <gtest/gtest.h>
#include "../src/minheap/MinHeap.h"

int TestTheTests()
{
  return 42;
}

TEST(Testing, Test)
{

  EXPECT_EQ(TestTheTests(), 42);

}

TEST(MinHeap, HighestElement)
{

  MinHeap<int> minHeap;

  minHeap.push(6);
  minHeap.push(10);
  minHeap.push(-3);

  EXPECT_EQ(minHeap.top(), 10);

}
