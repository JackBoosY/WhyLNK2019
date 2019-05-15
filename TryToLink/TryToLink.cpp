// TryToLink.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <process.h>

int main(int argc, char** argv)
{
	for (auto i = 1; i < argc; i++)
	{
		if (argv[i])
			printf("%s\n", argv[i]);
	}

	system("pause");
	return 0;
}