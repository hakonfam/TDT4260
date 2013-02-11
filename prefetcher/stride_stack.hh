#ifndef _STRIDE_STACK_H_
#define _STRIDE_STACK_H_

class StrideStack
{
 private:
  unsigned int currentSize, index;
  unsigned int[5] stack;
 public:
  void push(int value);
  unsigned int pop();
  unsigned int getCurrentSize();
}

#endif
