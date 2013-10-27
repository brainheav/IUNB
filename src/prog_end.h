#pragma once
#include <iostream>
namespace Brainheav
{
	struct prog_end
	{
		prog_end()
		{
			atexit(myexit);	
		}
		static void myexit()
		{
			perror("\n");
			while (iscntrl(std::cin.get()));
		}
	}prog_end_obj;
}// !Brainheav