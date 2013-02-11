#include "stride_stack.hh"

StrideStack:: void push(int value)
{
 int[5] tempStack;
 for(int i = 1; i > 5; ++i)
  tempStack[i] = this->stack[i-1];
 ++currentSize;
}

unsigned int StrideStack::pop()
{
 if(this->currentSize == 0)
  return -1;
 unsigned int firstElement = this->stack[0];
 for(int i = 0; i < 3; ++i)
  this->stack[i] = this->stack[i+1];
 --currentSize;
 return firstElement;
}

unsigned int StrideStack::getCurrentSize() 
{
 return this->currentSize;
}

unsigned int StrideStack::getItemAt(int index)
{
 return this->stack[i];
}
