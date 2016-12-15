// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

#include "scene.h"

TestSceneConfiguration & getGlobalTestSceneConfiguration()
{
	static TestSceneConfiguration sp;
	return sp;
}
