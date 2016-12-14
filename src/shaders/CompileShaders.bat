glslangValidator.exe -V forwardplus.vert -o ../../content/forwardplus_vert.spv
glslangValidator.exe -V forwardplus.frag -o ../../content/forwardplus_frag.spv
glslangValidator.exe -V light_culling.comp.glsl -o ../../content/light_culling_comp.spv -S comp
glslangValidator.exe -V depth.vert -o ../../content/depth_vert.spv

REM TODO: I want to do this in python... once I have more shaders to compile
REM like  "python compile_shaders.py?"
