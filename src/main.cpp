#include "VulkanShowBase.h"

#include <stdexcept>
#include <iostream>

int main() 
{
	VulkanShowBase app;

	try 
	{
		app.run();
	}
	catch (const std::runtime_error& e) 
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


