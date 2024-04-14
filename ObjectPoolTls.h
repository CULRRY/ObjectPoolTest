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
		int32 freeSize = 0;
	};

public:
	template <typename... Args>
	ObjectPoolTls(int32 initBuckets = 0, Args&&... args);
	~ObjectPoolTls();

	template <typename... Args>
	T*			Alloc(Args&&... args);
	void		Free(T* ptr);

	int32		GetCapacity() { return _bucketCapacity * BUCKET_SIZE; }
	int32		GetUseCount() { return _bucketUseCount * BUCKET_SIZE; }
	int32		EmptyBucket() { return _emptyBucketSize; }

private:
	template <typename ... Args>
	BucketNode*		create_bucket(Args&&... args);
	template <typename... Args>
	BucketNode*		alloc_bucket(Args&&... args);
	void			free_bucket(BucketNode* ptr);

	BucketNode*		get_empty_bucket();
	void			return_empty_bucket(BucketNode* bucket);

	TlsPool&		get_tls_pool()
	{
		TlsPool* pool = (TlsPool*)TlsGetValue(_tlsIndex);

		if (pool == nullptr)
		{
			pool = new TlsPool;
			TlsSetValue(_tlsIndex, pool);
		}

		return *pool;
	}

private:
	BucketNode* _head;

	BucketNode* _emptyBucketList;

	int32 _bucketUseCount;
	int32 _bucketCapacity;
	int32 _emptyBucketSize;

	Lock _lock;

	int32 _tlsIndex;
};

#include "ObjectPoolTls.hpp"
