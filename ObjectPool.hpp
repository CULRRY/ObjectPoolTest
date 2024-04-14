#pragma once
#include "ObjectPool.h"


template <typename T, bool IsConstructorCalled>
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
/// @brief Constructor
///
/// @param initBlocks	초기 블럭 개수
/// @param args			생성자에 들어갈 인자목록
///
//-------------------------------------------------------------------------------
template <typename... Args>
ObjectPool<T, IsConstructorCalled>::ObjectPool(int32 initBlocks, Args&&... args) :
	_head(nullptr), _capacity(initBlocks), _useCount(0), _id(0)
{
	//-----------------------------------------------------------------------------------------------
	/// 현재 OS에서 UserSpace 주소로 64비트를 전부다 사용하지 않기 때문에, 포인터에 해당 비트에
	///	ID를 숨겨놓을 수 있다. 해당 방식으로 ABA문제를 해결할 것인데, 현재 버전에서는 17비트를 쓸수
	///	있다. 하지만 OS 버전이 바뀌면 해당 비트의 갯수가 달라질 수 있으므로, 해당 코드에서 CRASH가
	///	난다면 NUM_OF_ID_BITS의 값을 현재 OS사양에 맞게 변경해야 한다.
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	const uint64 userSpaceMaximumAddress = reinterpret_cast<uint64>(info.lpMaximumApplicationAddress);

	CRASH_ASSERT((userSpaceMaximumAddress >> (64 - NUM_OF_ID_BITS)) == 0);
	//-----------------------------------------------------------------------------------------------

	for (int32 i = 0; i < initBlocks; i++)
	{
		BlockNode* ptr = reinterpret_cast<BlockNode*>(new BYTE[sizeof(BlockNode)]);

		// 생성자 호출옵션이 꺼져있으면, 맨처음 생성할 때 한번은 생성자가 호출되어야 한다.
		if constexpr (IsConstructorCalled == false)
		{
			new(ptr)T(args...);
		}

#ifdef MEMORY_CHECK
		ptr->paddingFront = this;
		ptr->paddingRear = this;
#endif

		// node push
		const uint64 id = InterlockedIncrement64(reinterpret_cast<LONG64*>(&_id));

		ptr->next = _head;
		_head = mergeIdIntoAddress(_head, id);
		
	}

}



//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
/// @brief 현재 FreeList에 있는 블럭들만 대상으로 메모리 해제를 진행한다.
///
//-------------------------------------------------------------------------------
template <typename T, bool IsConstructorCalled>
ObjectPool<T, IsConstructorCalled>::~ObjectPool()
{
	BlockNode* node = removeIdFromAddress(_head);

	while (node != nullptr)
	{
		BlockNode* next = removeIdFromAddress(node->next);

		if constexpr (IsConstructorCalled == true)
		{
			::free(node);
		}
		else
		{
			delete node;
		}

		node = next;
	}
}



template <typename T, bool IsConstructorCalled>
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
/// @brief 메모리를 Pool에서 꺼내온다. 없을경우 할당한다.
///
/// @param args		생성자에 들어갈 인자목록
/// @return			할당된 포인터
///
//-------------------------------------------------------------------------------
template <typename... Args>
T* ObjectPool<T, IsConstructorCalled>::Alloc(Args&&... args)
{
	BlockNode* ptr;

	// pop node

	for(;;)
	{
		BlockNode* returnNode = _head;
		if (returnNode == nullptr)
		{
			ptr = nullptr;
			break;
		}

		ptr = removeIdFromAddress(returnNode);

		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_head), ptr->next, returnNode) == returnNode)
		{
			break;
		}
	}


	if (ptr == nullptr) /* Pool이 비어있다면 노드를 새로 동적할당*/
	{
		ptr = reinterpret_cast<BlockNode*>(new BYTE[sizeof(BlockNode)]);

#ifdef MEMORY_CHECK
		ptr->paddingFront = this;
		ptr->paddingRear = reinterpret_cast<BYTE*>(this) + 1;
		ptr = reinterpret_cast<BlockNode*>(reinterpret_cast<BYTE*>(ptr) + sizeof(void*));
#endif

		new(ptr)T(args...); // 생성자 호출

		InterlockedIncrement(reinterpret_cast<LONG*>(&_capacity));
	}
	else
	{
#ifdef MEMORY_CHECK
		ptr->paddingFront = this;
		ptr->paddingRear = reinterpret_cast<BYTE*>(ptr->paddingRear) + 1;
		ptr = reinterpret_cast<BlockNode*>(reinterpret_cast<BYTE*>(ptr) + sizeof(void*));
#endif

		if constexpr (IsConstructorCalled == true)
		{
			new(ptr)T(args...); // 생성자 호출
		}

	}

	InterlockedIncrement(reinterpret_cast<LONG*>(&_useCount));

	return reinterpret_cast<T*>(ptr);
}



//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
/// @brief 메모리를 Pool에 반환한다.
///
/// @param ptr 해제할 메모리 시작주소
///
//-------------------------------------------------------------------------------
template <typename T, bool IsConstructorCalled>
void ObjectPool<T, IsConstructorCalled>::Free(T* ptr)
{
	if constexpr (IsConstructorCalled == true)
	{
		ptr->~T(); // 소멸자 호출
	}

	BlockNode* block = reinterpret_cast<BlockNode*>(ptr);
#ifdef MEMORY_CHECK
	block = reinterpret_cast<BlockNode*>(reinterpret_cast<BYTE*>(block) - sizeof(void*));

	// padding이 손상되었다면, CRASH
	CRASH_ASSERT(block->paddingFront == this && block->paddingRear == reinterpret_cast<BYTE*>(this) + 1)

	block->paddingRear = reinterpret_cast<BYTE*>(block->paddingRear) - 1;
#endif


	// push node
	const uint64 id = InterlockedIncrement64(reinterpret_cast<LONG64*>(&_id));

	BlockNode* newTopPtr = mergeIdIntoAddress(block, id);
	for (;;)
	{
		BlockNode* top = _head;
		block->next = top;
		if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_head), newTopPtr, top) == top)
		{
			InterlockedDecrement(reinterpret_cast<LONG*>(&_useCount));
			break;
		}
	}
	
}
