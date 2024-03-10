#pragma once

#include <cstdint>

namespace leap
{
	class MemoryTracker final
	{
	public:
		static void TrackMemory(const uint64_t nrOfBytes);
		static void UntrackMemory(const uint64_t nrOfBytes);

	private:
		inline static uint64_t m_TotalAllocatedMemory;
	};
}