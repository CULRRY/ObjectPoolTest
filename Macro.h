#pragma once


#define CRASH(str) __debugbreak();
#define CRASH_ASSERT(expr) if ((expr) == false) __debugbreak();


#define CONCATMSG(a, b) a##b
#define CONCAT(a,b) CONCATMSG(a, b)
