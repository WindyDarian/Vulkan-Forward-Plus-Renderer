# Vulkan Forward+ Renderer using compute shader

A Forward+ rendering pipeline in Vulkan. A final project for __CIS565: GPU Programming__ at University of Pennsylvania

WORK IN PROGRESS!

No forward+ renderer is here yet, this project is just starting...

Members:
* [Ruoyu Fan](http://windy.moe/portfolio)
* [Xueyin Wan](https://github.com/xueyinw)

### build

Use CMake to build the program.

#### Windows
Make sure you have Vulkan SDK and Visual Studio 2015 or up, then:
```
mkdir build
cd build
cmake-gui ..
```
And `Configure`(select `"Visual Studio 2015 x64"`), `Generate`, then you have Visual Studio project files.

Remember to copy `src/content` to `build` folder in order to run it.


#### Linux
Make sure `VULKAN_SDK` is set to `x86_64` folder under Vulkan SDK path and you have `LD_LIBRARY_PATH` and `VK_LAYER_PATH` set by running `source ./setup-env.sh` at Vulkan SDK folder, and then
```
mkdir build
cd build
cmake ..
make
```
copy `src/content` to `build` folder, then run the executable
