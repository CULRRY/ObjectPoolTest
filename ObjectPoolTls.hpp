#pragma once
#include "ObjectPoolTls.h"

template <typename T, bool IsConstructorCalled>
template <typename ... Args>
ObjectPoolTls<T, IsConstructorCalled>::ObjectPoolTls(int32 initBuckets, Args&&... args)
	: _head(nullptr)
	, _emptyBucketList(nullptr)
	, _registeredPools{ nullptr, }
	, _registeredPoolsCount(0)
	, _capacity(0)
	, _tlsIndex(TlsAlloc())
{
	for (int32 i = 0; i < initBuckets; i++)
	{
		BucketNode* bucket = create_bucket(args...);
		bucket->next = _head;
		_head = bucket;
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
		Node* node = bucket->head;
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

	Bucket* bucket = &pool.useBucket;

	if (pool.useBucket == nullptr)
	{
		if (pool.freeBucket == nullptr)
		{
			pool.useBucket = alloc_bucket(args...);
		}
		else
		{
			bucket = &pool.freeBucket;
		}
	}

	Node* ret = *bucket;
	*bucket = (*bucket)->next;

	if constexpr (IsConstructorCalled == true)
	{
		new(ret)T(args...);
	}

	pool.useCount++;

	return (T*)ret;
}

template <typename T, bool IsConstructorCalled>
void ObjectPoolTls<T, IsConstructorCalled>::Free(T* ptr)
{
	TlsPool& pool = get_tls_pool();

	if constexpr (IsConstructorCalled == true)
	{
		ptr->~T(); // 소멸자 호출
	}

	Node* node = reinterpret_cast<Node*>(ptr);

	node->next = pool.freeBucket;
	pool.freeBucket = node;
	--pool.useCount;
	++pool.freeSize;

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

		ptr->next = bucket->head;
		bucket->head = ptr;
	}

	_capacity += BUCKET_SIZE;
	return bucket;
}

template <typename T, bool IsConstructorCalled>
template <typename... Args>
typename
ObjectPoolTls<T, IsConstructorCalled>::Bucket ObjectPoolTls<T, IsConstructorCalled>::alloc_bucket(Args&&... args)
{
	WRITE_LOCK(_lock);
	BucketNode* bucket;

	if (_head == nullptr)
	{
		bucket = create_bucket(args...);
	}
	else
	{
		bucket = _head;
		_head = _head->next;
	}

	Bucket ret = bucket->head;
	bucket->head = nullptr;
	return_empty_bucket(bucket);

	return ret;
}

template <typename T, bool IsConstructorCalled>
void ObjectPoolTls<T, IsConstructorCalled>::free_bucket(Bucket ptr)
{
	WRITE_LOCK(_lock);

	BucketNode* bucket = get_empty_bucket();
	bucket->head = ptr;

	bucket->next = _head;
	_head = bucket;
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
	}
	bucket->head = nullptr;
	bucket->next = nullptr;

	return bucket;
}

template <typename T, bool IsConstructorCalled>
void ObjectPoolTls<T, IsConstructorCalled>::return_empty_bucket(BucketNode* bucket)
{
	bucket->next = _emptyBucketList;
	_emptyBucketList = bucket;
}
