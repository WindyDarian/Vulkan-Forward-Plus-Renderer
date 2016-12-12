glslangValidator.exe -V helloworld.vert -o ../../content/helloworld_vert.spv
glslangValidator.exe -V helloworld.frag -o ../../content/helloworld_frag.spv
glslangValidator.exe -V light_culling.comp.glsl -o ../../content/light_culling.comp.spv -S comp
glslangValidator.exe -V depth.vert -o ../../content/depth_vert.spv

REM TODO: I want to do this in python... once I have more shaders to compile
REM like  "python compile_shaders.py?"
