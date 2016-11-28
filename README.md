# VFPR - a Vulkan Forward+ Renderer using compute shader

A Forward+ (tiled forward) rendering pipeline in Vulkan. A final project for __CIS565: GPU Programming__ at University of Pennsylvania

WORK IN PROGRESS!

No forward+ renderer is here yet, this project is just starting...

Members:
* [Ruoyu Fan](http://windy.moe/portfolio)
* [Xueyin Wan](https://github.com/xueyinw)

## TODOs
* Split VulkanShowBase into ShowBase and VulkanContext
* Camera control
* Point Light

## Milestones

### Camera control, point lights, normal mapping and Blinn-Phong shading  (11/28/2016)

![](/screenshots/blinn-phong_and_normal_mapping.jpg)

### Milestone 1 (11/21/2016)
* Basic Vulkan framework including:
  * basic forward rendering pipeline (adaptd from [Vulkan Tutorial by Alexander Overvoorde](https://vulkan-tutorial.com))
  * mesh
  * texture mapping
  * lambert shading

![](/screenshots/lambertian.jpg)

## How to Build

Use CMake to build the program.

#### Windows
1. Make sure you have Vulkan SDK and Visual Studio 2015 or up, then:
```
mkdir build
cd build
cmake-gui ..
```
2. And `Configure`(select `"Visual Studio 2015 x64"`), `Generate`, then you have Visual Studio project files.

3. Set `vfpr` as startup project and build solution


#### Linux
Make sure `VULKAN_SDK` is set to `x86_64` folder under Vulkan SDK path and you have `LD_LIBRARY_PATH` and `VK_LAYER_PATH` set by running `source ./setup-env.sh` at Vulkan SDK folder, and then
```
mkdir build
cd build
cmake ..
make
```


## Third-Party Credits

#### References
* [Vulkan Tutorial by Alexander Overvoorde](https://vulkan-tutorial.com)

#### Libraries
* [CMake](https://cmake.org/)
* [GLFW](http://www.glfw.org/)
* [GLM](http://glm.g-truc.net/0.9.8/index.html)
* [stb_image](https://github.com/nothings/stb)
* [tinyobjloader](https://github.com/syoyo/tinyobjloader)

#### Assets
* [Chalet Hippolyte Chassande Baroz by Gaël](https://skfb.ly/HDVU)
* [Sponza modeled by Marko Dabrovic](http://hdri.cgtechniques.com/~sponza/files/Sponza) (CIS565 simplified version) 
