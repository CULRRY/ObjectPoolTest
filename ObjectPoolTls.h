#pragma once
#include "Lock.h"


template<typename T, bool IsConstructorCalled = true>
class ObjectPoolTls
{
	enum
	{
		BUCKET_SIZE = 5000,
	};


	struct Node
	{
		T data;
		Node* next = nullptr;
	};

	struct BucketNode
	{
		Node* node = nullptr;
		BucketNode* next;
	};


	struct TlsPool
	{
		BucketNode* useBucket = nullptr;
		BucketNode* freeBucket = nullptr;
		int32 useCount = 0;
		int32 freeSize = 0;
	};

public:
	template <typename... Args>
	ObjectPoolTls(int32 initBuckets = 0, Args&&... args);
	~ObjectPoolTls();

	template <typename... Args>
	T* Alloc(Args&&... args);
	void		Free(T* ptr);

	int32		GetCapacity() { return _capacity; }
	int32		GetUseCount()
	{
		int32 ret = 0;
		for (int32 i = 0; i < _registeredPoolsCount; i++)
		{
			ret += _registeredPools[i]->useCount;
		}
		return ret;
	}

private:
	template <typename ... Args>
	BucketNode* create_bucket(Args&&... args);
	template <typename... Args>
	BucketNode* alloc_bucket(Args&&... args);
	void			free_bucket(BucketNode* ptr);

	BucketNode* get_empty_bucket();
	void			return_empty_bucket(BucketNode* bucket);

	TlsPool& get_tls_pool()
	{
		TlsPool* pool = (TlsPool*)TlsGetValue(_tlsIndex);

		if (pool == nullptr)
		{
			pool = new TlsPool;
			TlsSetValue(_tlsIndex, pool);
			int32 idx = InterlockedIncrement((LONG*)&_registeredPoolsCount) - 1;
			_registeredPools[idx] = pool;
		}

		return *pool;
	}

private:
	BucketNode* _head;

	BucketNode* _emptyBucketList;


	TlsPool* _registeredPools[64];
	int32	_registeredPoolsCount;

	LONG _capacity = 0;

	Lock _lock;

	int32 _tlsIndex;
};

#include "ObjectPoolTls.hpp"
