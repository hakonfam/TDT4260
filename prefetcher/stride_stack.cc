#include "stride_stack.hh"
#include <iostream>

void StrideStack::push(int value)
{
 for(int i = 4; i > 0; --i)
  this->stack[i] = this->stack[i-1];
 
 if(currentSize < 5) currentSize++;

 this->stack[0] = value;
}

unsigned int StrideStack::getCurrentSize() 
{
 return this->currentSize;
}

unsigned int StrideStack::getItemAt(int index)
{
 return this->stack[index];
}

void StrideStack::printStack()
{
 std::cout << "Current stack: \n";
 for(int i = 0; i < currentSize; ++i)
 {
 std::cout << i << ": " << stack[i] << "\n";
 }
	std::cout << "\n\n";
}
