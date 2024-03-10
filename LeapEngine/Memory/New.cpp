#include "Mallocator.h"

#include <stdlib.h>

void* operator new(size_t nrOfBytes)
{
	if (nrOfBytes == 0)
	{
		// standard dictates that *every* request should return a valid pointer,
		// so make sure we allocate a minimum of 1 byte
		nrOfBytes = 1;
	}

	return leap::Mallocator::GetInstance().Allocate(static_cast<int32_t>(nrOfBytes));
}

void operator delete(void* pMemory)
{
	if (!pMemory)
	{
		return;
	}

	leap::Mallocator::GetInstance().Deallocate(pMemory);
}