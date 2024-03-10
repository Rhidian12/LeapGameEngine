#include "Mallocator.h"
#include "MemoryTracker.h"

#include <algorithm>
#include <assert.h>

#define SAFE_DELETE(pointer) free(pointer); pointer = nullptr;

namespace leap
{
	static const uint16_t MEMORY_TAG = 0xBEEF;

	Mallocator::Mallocator()
		: m_pBlocks{}
		, m_pCurrentEnd{}
	{}

	Mallocator::~Mallocator()
	{
		Node* pNode{ m_pBlocks };

		while (pNode)
		{
			Node* pNodeToRemove{ pNode };

			pNode = pNode->pNext;

			SAFE_DELETE(pNodeToRemove->pBlock);
			SAFE_DELETE(pNodeToRemove);
		}
	}

	void* Mallocator::Allocate(const int32_t nrOfBytes)
	{
		assert(nrOfBytes > 0);

		// Allocate a Block + the actual memory we're trying to allocate
		Block* pNewBlock{ static_cast<Block*>(malloc(sizeof(Block) + nrOfBytes)) };

		assert(pNewBlock != nullptr);

		pNewBlock->tag = MEMORY_TAG;
		pNewBlock->size = nrOfBytes;
		pNewBlock->pBlock = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(pNewBlock) + sizeof(Block));

		AddNewBlock(pNewBlock);

		MemoryTracker::TrackMemory(nrOfBytes);

		return pNewBlock->pBlock;
	}

	void Mallocator::Deallocate(void* pMemory)
	{
		// check if this piece of memory is part of our memory
		// we do this by taking the memory address and going BACK to the tag to check if it's 0xBEEF
		Block* const pBlock{ reinterpret_cast<Block*>(reinterpret_cast<uint64_t>(pMemory) - sizeof(Block::size) - sizeof(Block::tag)) };
		const uint16_t tag{ pBlock->tag };

		if (tag != MEMORY_TAG)
		{
			return;
		}

		MemoryTracker::UntrackMemory(pBlock->size);

		RemoveExistingBlock(pBlock);
	}

	void Mallocator::AddNewBlock(Block* const pBlock)
	{
		Node* const pNewNode{ static_cast<Node*>(malloc(sizeof(Node))) };

		pNewNode->pBlock = pBlock;
		pNewNode->pNext = nullptr;

		if (!m_pBlocks)
		{
			m_pBlocks = pNewNode;
		}

		if (!m_pCurrentEnd)
		{
			m_pCurrentEnd = pNewNode;
		}
		else
		{
			m_pCurrentEnd->pNext = pNewNode;
			m_pCurrentEnd = pNewNode;
		}

		++m_NrOfBlocks;
	}

	void Mallocator::RemoveExistingBlock(Block* const pBlock)
	{
		if (!m_pBlocks)
		{
			// no point in searching if we have not allocated
			return;
		}

		// first, do a trivial check for the beginning and end of our list
		if (Node* pStart{ m_pBlocks }; pStart->pBlock == pBlock)
		{
			m_pBlocks = pStart->pNext;

			SAFE_DELETE(pStart->pBlock);
			SAFE_DELETE(pStart);

			--m_NrOfBlocks;

			return;
		}
		else if (Node* pEnd{ m_pCurrentEnd }; pEnd->pBlock == pBlock)
		{
			// traverse all the way to the second last node
			Node* pNewEnd{ m_pBlocks };
			while (pNewEnd->pNext != pEnd)
			{
				pNewEnd = pNewEnd->pNext;
			}

			m_pCurrentEnd = pEnd;

			SAFE_DELETE(pEnd->pBlock);
			SAFE_DELETE(pEnd);

			--m_NrOfBlocks;

			return;
		}

		// we've already checked the first node, so we can start at the second node
		Node* pPreviousNode{ m_pBlocks };
		Node* pNode{ m_pBlocks->pNext };

		while (pNode)
		{
			if (pNode->pBlock == pBlock)
			{
				// we found our block, we want to remove this node and block
				// so, we need to link the previous block to the next block
				pPreviousNode->pNext = pNode->pNext;

				SAFE_DELETE(pNode->pBlock);
				SAFE_DELETE(pNode);

				--m_NrOfBlocks;

				return;
			}

			pPreviousNode = pNode;
			pNode = pNode->pNext;
		}
	}
}