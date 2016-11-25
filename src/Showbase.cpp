// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "Showbase.h"

#include "renderer/VulkanShowBase.h"

#include <GLFW/glfw3.h>

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
const bool WINDOW_RESIZABLE = true;

class _ShowBase_Impl
{
private:
	VulkanShowBase context;

public:
	void run()
	{
		context.run();
	}
};

void ShowBase::run()
{
	p_impl->run();
}

ShowBase::ShowBase()
{
	p_impl = std::make_unique<_ShowBase_Impl>();
}

ShowBase::~ShowBase() = default; 
