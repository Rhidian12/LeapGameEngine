#include "Mallocator.h"

#include <stdlib.h>

leap::Mallocator& GetMallocator()
{
	static leap::Mallocator mallocator{};
	return mallocator;
}

void* operator new(size_t nrOfBytes)
{
	if (nrOfBytes == 0)
	{
		// standard dictates that *every* request should return a valid pointer,
		// so make sure we allocate a minimum of 1 byte
		nrOfBytes = 1;
	}

	return GetMallocator().Allocate(static_cast<int32_t>(nrOfBytes));
}

void operator delete(void* pMemory)
{
	if (!pMemory)
	{
		return;
	}

	GetMallocator().Deallocate(pMemory);
}