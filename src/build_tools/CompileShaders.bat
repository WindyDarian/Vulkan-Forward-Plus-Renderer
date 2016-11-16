md "../content"
%VK_SDK_PATH%\Bin\glslangValidator.exe -V ../shaders/helloworld.vert -o ../content/helloworld_vert.spv
%VK_SDK_PATH%\Bin\glslangValidator.exe -V ../shaders/helloworld.frag -o ../content/helloworld_frag.spv

REM TODO: I want to do this in python... once I have more shaders to compile
REM like  "python compile_shaders.py?"
