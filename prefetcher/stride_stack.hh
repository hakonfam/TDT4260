#ifndef _STRIDE_STACK_H_
#define _STRIDE_STACK_H_
#include "prefetcher.hh"	
class StrideStack
{
 private:
  unsigned int currentSize, index;
  unsigned int stack[5];
 public:
  void push(Addr value);
	void printStack();
  unsigned int getCurrentSize();
	unsigned int getItemAt(int index);
};

#endif
