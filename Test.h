#pragma once
#include <process.h>
#include "Lock.h"
#include "ObjectPoolTls.h"

#define TEST(dataSize) Test<dataSize> test##dataSize

template <int32 Size>
class Test
{
	enum
	{
		NUM_THREAD = 12,
	};

	struct DummyData
	{
		BYTE data[Size];
	};

	using FuncType	= unsigned(*)(void*);
	using DataPtr	= DummyData*;


public:
	Test()
	{
		RunThread(BenchmarkPoolAllocFree);
		RunThread(BenchmarkHeapAllocFree);

		printf("Data Size<%d>\n", Size);
		printf("Pool : %10.1llf us\n", GetAveragePoolTime());
		printf("Heap : %10.1llf us\n", GetAverageHeapTime());
		printf("Faster Than x%.2llf\n\n", GetAverageHeapTime() / (int64)GetAveragePoolTime());
	}

	static void RunThread(FuncType func)
	{
		HANDLE t[NUM_THREAD];
		readyCount = 0;

		for (int i = 0; i < NUM_THREAD; i++)
		{
			t[i] = (HANDLE)_beginthreadex(nullptr, 0, func, nullptr, 0, nullptr);
			SetThreadPriority(t[i], THREAD_PRIORITY_HIGHEST);
		}

		WaitForMultipleObjects(NUM_THREAD, t, true, INFINITE);
		for (int i = 0; i < NUM_THREAD; i++)
		{
			CloseHandle(t[i]);
		}
	}

	static void WarmUpPool(DataPtr* arr)
	{
		for (int i = 0; i < 300000; i++)
		{
			arr[i] = pool.Alloc();
		}

		for (int i = 0; i < 300000; i++)
		{
			pool.Free(arr[i]);
		}

		InterlockedIncrement(&readyCount);
		while (readyCount != NUM_THREAD);
	}

	static void WarmUpHeap(DataPtr* arr)
	{
		for (int i = 0; i < 300000; i++)
		{
			arr[i] = new DummyData;
		}

		for (int i = 0; i < 300000; i++)
		{
			delete arr[i];
		}

		InterlockedIncrement(&readyCount);
		while (readyCount != NUM_THREAD);
	}

	static unsigned WINAPI BenchmarkPoolAllocFree(void* param)
	{
		DataPtr* arr = new DataPtr[300000];
		LARGE_INTEGER start;
		LARGE_INTEGER end;

		WarmUpPool(arr);

		QueryPerformanceCounter(&start);
		for (int i = 0; i < 100000; i++)
		{
			arr[i] = pool.Alloc();
		}

		for (int i = 0; i < 100000; i++)
		{
			pool.Free(arr[i]);
		}
		QueryPerformanceCounter(&end);

		WRITE_LOCK(lock);
		totalPoolAllocFreeTime.QuadPart += end.QuadPart - start.QuadPart;

		return 0;
	}

	static unsigned WINAPI BenchmarkHeapAllocFree(void* param)
	{

		DataPtr* arr = new DataPtr[300000];
		LARGE_INTEGER start;
		LARGE_INTEGER end;

		WarmUpHeap(arr);

		QueryPerformanceCounter(&start);
		for (int i = 0; i < 100000; i++)
		{
			arr[i] = new DummyData;
		}

		for (int i = 0; i < 100000; i++)
		{
			delete arr[i];
		}
		QueryPerformanceCounter(&end);

		WRITE_LOCK(lock);
		totalHeapAllocFreeTime.QuadPart += end.QuadPart - start.QuadPart;

		return 0;
	}

	static double GetAveragePoolTime()
	{
		return (double)(totalPoolAllocFreeTime.QuadPart / NUM_THREAD) / 10;
	}

	static double GetAverageHeapTime()
	{
		return (double)(totalHeapAllocFreeTime.QuadPart / NUM_THREAD) / 10;
	}


private:
	inline static ObjectPoolTls<DummyData, false> pool;

	inline static LARGE_INTEGER totalPoolAllocFreeTime{};
	inline static LARGE_INTEGER totalHeapAllocFreeTime{};

	inline static LONG readyCount = 0;

	inline static Lock lock;
};

