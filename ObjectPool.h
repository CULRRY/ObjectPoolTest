#pragma once
enum : uint64
{
	NUM_OF_ID_BITS = 17,
	ADDRESS_MASK = UINT64_MAX >> NUM_OF_ID_BITS,
};

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
/// @class		ObjectPool
/// @brief		Lock-Free Object Pool
/// @details	MEMORY_CHECK를 Define하면, 포인터를 할당, 해제, 사용할 때 안전장치를
///				동작시킬 수 있음.
///
/// @tparam T					데이터 타입
/// @tparam IsConstructorCalled	할당, 해제시 생성자, 소멸자 호출여부
///
//-------------------------------------------------------------------------------
template <typename T, bool IsConstructorCalled = true>
class ObjectPool
{
	struct BlockNode
	{
#ifdef MEMORY_CHECK
		void* paddingFront; /// PADDING
#endif
		T data;
#ifdef MEMORY_CHECK
		void* paddingRear;	/// PADDING
#endif
		BlockNode* next;
	};

public:
	template <typename... Args>
	ObjectPool(int32 initBlocks = 0, Args&&... args);
	virtual ~ObjectPool();

	template <typename... Args>
	T*			Alloc(Args&&... args);
	void		Free(T* ptr);

	int32		GetCapacity() { return _capacity; }
	int32		GetUseCount() { return _useCount; }

private:
	static BlockNode* removeIdFromAddress(void* ptr)
	{
		return reinterpret_cast<BlockNode*>(reinterpret_cast<uint64>(ptr) & ADDRESS_MASK);
	}
	static BlockNode* mergeIdIntoAddress(void* ptr, uint64 id)
	{
		return reinterpret_cast<BlockNode*>(reinterpret_cast<uint64>(ptr) | (id << (64 - NUM_OF_ID_BITS)));
	}

private:

	BlockNode*	_head;					///< 스택 헤드

	int32		_capacity;				///< 현재 확보 된 블럭 개수
	int32		_useCount;				///< 현재 사용중인 블럭 개수
	uint64		_id;
};

#include "ObjectPool.hpp"