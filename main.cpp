#include "stdafx.h"

#include <string>
#include <filesystem>
#include "Test.h"

int main(int argc, char* argv[])
{

	if (filesystem::exists("./ObjPoolBenchmark.csv") == false)
	{
		fstream file("ObjPoolBenchmark.csv", ios::out);
		file << "size,heap,pool" << endl;
		file.close();
	}

	int size = stoi(string(argv[1]));

	switch (size)
	{
	case 1:
		{
		TEST(1);
		break;
		}

	case 2:
		{
			TEST(2);
			break;
		}
	case 4:
		{
			TEST(4);
			break;
		}
	case 8:
		{
			TEST(8);
			break;
		}
	case 16:
		{
			TEST(16);
			break;
		}
	case 32:
		{
			TEST(32);
			break;
		}
	case 64:
		{
			TEST(64);
			break;
		}
	case 128:
		{
			TEST(128);
			break;
		}
	case 256:
		{
			TEST(256);
			break;
		}
	case 512:
		{
			TEST(512);
			break;
		}
	case 1024:
		{
			TEST(1024);
			break;
		}
	case 2048:
		{
			TEST(2048);
			break;
		}
	}


	// TEST(1);
	// TEST(2);
	// TEST(4);
	// TEST(8);
	// TEST(16);
	// TEST(32);
	// TEST(64);
	// TEST(128);
	// TEST(256);
	// TEST(512);
	// TEST(1024);
	// TEST(2048);

	return 0;
}