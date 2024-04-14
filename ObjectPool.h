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
/// @details	MEMORY_CHECK�� Define�ϸ�, �����͸� �Ҵ�, ����, ����� �� ������ġ��
///				���۽�ų �� ����.
///
/// @tparam T					������ Ÿ��
/// @tparam IsConstructorCalled	�Ҵ�, ������ ������, �Ҹ��� ȣ�⿩��
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

	BlockNode*	_head;					///< ���� ���

	int32		_capacity;				///< ���� Ȯ�� �� �� ����
	int32		_useCount;				///< ���� ������� �� ����
	uint64		_id;
};

#include "ObjectPool.hpp"