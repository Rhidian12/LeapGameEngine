#pragma once

#include <cstdint>
#include <vector>

namespace leap
{
	/// <summary>
	/// Mallocator is a simple malloc() based allocator, with tags for memory tracking
	/// </summary>
	class Mallocator final
	{
	private:
		struct Block
		{
			uint16_t tag;
			int32_t size;
			void* pBlock;
		};

		struct Node
		{
			Block* pBlock;
			Node* pNext;
		};

	public:
		Mallocator();
		~Mallocator();

		void* Allocate(const int32_t nrOfBytes);
		void Deallocate(void* pMemory);

	private:
		void AddNewBlock(Block* const pBlock);
		void RemoveExistingBlock(Block* const pBlock);

		Node* m_pBlocks;
		Node* m_pCurrentEnd;

		int32_t m_NrOfBlocks;
	};
}