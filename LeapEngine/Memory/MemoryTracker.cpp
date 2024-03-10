#include "MemoryTracker.h"

namespace leap
{
	void MemoryTracker::TrackMemory(const uint64_t nrOfBytes)
	{
		m_TotalAllocatedMemory += nrOfBytes;
	}

	void MemoryTracker::UntrackMemory(const uint64_t nrOfBytes)
	{
		m_TotalAllocatedMemory -= nrOfBytes;
	}
}