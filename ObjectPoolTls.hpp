#pragma once
#include "ObjectPoolTls.h"

template <typename T, bool IsConstructorCalled>
template <typename ... Args>
ObjectPoolTls<T, IsConstructorCalled>::ObjectPoolTls(int32 initBuckets, Args&&... args)
	: _head(nullptr)
	, _emptyBucketList(nullptr)
	, _bucketUseCount(0)
	, _bucketCapacity(0)
{

	_tlsIndex = TlsAlloc();

	for (int32 i = 0; i < initBuckets; i++)
	{
		BucketNode* bucket = create_bucket(args...);
	}
}

template <typename T, bool IsConstructorCalled>
ObjectPoolTls<T, IsConstructorCalled>::~ObjectPoolTls()
{
	BucketNode* bucket = _head;

	int32 bkcnt = 0;
	while (bucket != nullptr)
	{
		bkcnt++;
		BucketNode* bucketNext = bucket->next;
		Node* node = bucket->node;
		int32 cnt = 0;
		while (node != nullptr)
		{
			cnt++;
			Node* next = node->next;
			delete node;
			node = next;
		}
		delete bucket;
		bucket = bucketNext;
	}

	bucket = _emptyBucketList;
	while (bucket != nullptr)
	{
		BucketNode* bucketNext = bucket->next;
		delete bucket;
		bucket = bucketNext;
	}
}

template <typename T, bool IsConstructorCalled>
template <typename ... Args>
T* ObjectPoolTls<T, IsConstructorCalled>::Alloc(Args&&... args)
{
	TlsPool& pool = get_tls_pool();

	Node* ret;
	if (pool.useBucket == nullptr)
	{
		pool.useBucket = alloc_bucket(args...);
		ret = pool.useBucket->node;
		pool.useBucket->node = pool.useBucket->node->next;
	}
	else if (pool.useBucket->node != nullptr)
	{
		ret = pool.useBucket->node;
		pool.useBucket->node = pool.useBucket->node->next;
	}
	else if (pool.freeBucket != nullptr && pool.freeBucket->node != nullptr)
	{
		ret = pool.freeBucket->node;
		pool.freeBucket->node = pool.freeBucket->node->next;
	}
	else
	{
		return_empty_bucket(pool.useBucket);
		pool.useBucket = alloc_bucket(args...);
		ret = pool.useBucket->node;
		pool.useBucket->node = pool.useBucket->node->next;
	}

	if constexpr (IsConstructorCalled == true)
	{
		new(ret)T(args...);
	}

	return (T*)ret;
}

template <typename T, bool IsConstructorCalled>
void ObjectPoolTls<T, IsConstructorCalled>::Free(T* ptr)
{
	TlsPool& pool = get_tls_pool();

	if (pool.freeBucket == nullptr)
	{
		WRITE_LOCK(_lock);
		pool.freeBucket = get_empty_bucket();
	}

	if constexpr (IsConstructorCalled == true)
	{
		ptr->~T(); // 소멸자 호출
	}

	Node* node = reinterpret_cast<Node*>(ptr);

	node->next = pool.freeBucket->node;
	pool.freeBucket->node = node;
	pool.freeSize++;

	if (pool.freeSize == BUCKET_SIZE)
	{
		free_bucket(pool.freeBucket);
		pool.freeBucket = nullptr;
		pool.freeSize = 0;
	}
}

template <typename T, bool IsConstructorCalled>
template <typename ... Args>
typename
ObjectPoolTls<T, IsConstructorCalled>::BucketNode* ObjectPoolTls<T, IsConstructorCalled>::create_bucket(Args&&... args)
{
	BucketNode* bucket = get_empty_bucket();

	for (int32 i = 0; i < BUCKET_SIZE; i++)
	{
		Node* ptr = reinterpret_cast<Node*>(new BYTE[sizeof(Node)]);

		// 생성자 호출옵션이 꺼져있으면, 맨처음 생성할 때 한번은 생성자가 호출되어야 한다.
		if constexpr (IsConstructorCalled == false)
		{
			new(ptr)T(args...);
		}

		ptr->next = bucket->node;
		bucket->node = ptr;
	}

	++_bucketCapacity;

	return bucket;
}

template <typename T, bool IsConstructorCalled>
template <typename... Args>
typename
ObjectPoolTls<T, IsConstructorCalled>::BucketNode* ObjectPoolTls<T, IsConstructorCalled>::alloc_bucket(Args&&... args)
{
	WRITE_LOCK(_lock);

	BucketNode* bucket;

	if (_bucketCapacity - _bucketUseCount == 0)
	{
		bucket = create_bucket(args...);
	}
	else
	{
		bucket = _head;
		_head = _head->next;
	}

	++_bucketUseCount;

	return bucket;
}

template <typename T, bool IsConstructorCalled>
void ObjectPoolTls<T, IsConstructorCalled>::free_bucket(BucketNode* ptr)
{
	WRITE_LOCK(_lock);


	BucketNode* bucket = ptr;

	bucket->next = _head;
	_head = bucket;

	--_bucketUseCount;
}

template <typename T, bool IsConstructorCalled>
typename ObjectPoolTls<T, IsConstructorCalled>::BucketNode* ObjectPoolTls<T, IsConstructorCalled>::get_empty_bucket()
{
	BucketNode* bucket;

	if (_emptyBucketList == nullptr)
	{
		bucket = new BucketNode;
	}
	else
	{
		bucket = _emptyBucketList;
		_emptyBucketList = _emptyBucketList->next;
		bucket->next = nullptr;
		_emptyBucketSize--;
	}

	return bucket;
}

template <typename T, bool IsConstructorCalled>
inline void ObjectPoolTls<T, IsConstructorCalled>::return_empty_bucket(BucketNode* bucket)
{
	WRITE_LOCK(_lock);
	bucket->next = _emptyBucketList;
	_emptyBucketList = bucket;
	_emptyBucketSize++;
}
