#pragma once
#define __AUTO_GUARD_NAME CONCAT(__lock_guard, __COUNTER__)
#define WRITE_LOCK(lock) \
		LockGuard<EXCLUSIVE> __AUTO_GUARD_NAME(lock)
#define READ_LOCK(lock)	 \
		LockGuard<SHARED> __AUTO_GUARD_NAME(lock)


#pragma warning(disable: 26110)
class Lock
{

public:
	Lock()
	{
		InitializeSRWLock(&_lock);
	}

	void AcquireExclusive()
	{
		AcquireSRWLockExclusive(&_lock);
	}

	void ReleaseExclusive()
	{
		ReleaseSRWLockExclusive(&_lock);
	}

	void AcquireShared()
	{
		AcquireSRWLockShared(&_lock);
	}

	void ReleaseShared()
	{
		ReleaseSRWLockShared(&_lock);
	}

private:
	SRWLOCK _lock;
};


enum ModeType
{
	SHARED,
	EXCLUSIVE,
};

template <ModeType Mode>
class LockGuard
{
public:
	LockGuard(Lock& lock) : _lock(lock)
	{
		if constexpr (Mode == SHARED)
		{
			lock.AcquireShared();
		}
		else
		{
			lock.AcquireExclusive();
		}
	}

	~LockGuard()
	{
		if constexpr (Mode == SHARED)
		{
			_lock.ReleaseShared();
		}
		else
		{
			_lock.ReleaseExclusive();
		}
	}

	LockGuard() = delete;

private:
	Lock& _lock;

};
