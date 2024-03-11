#include <iostream>
#include "../src/minheap/MinHeap.h"

int main()
{

  MinHeap<int> minheap;

  minheap.push(5);
  minheap.push(6);
  minheap.push(-3);

  while (!minheap.empty())
  {

    std::cout << minheap.top() << "\n";

    minheap.pop();

  }

  std::cout << "Hello Testing\n";

  return 0;

}
