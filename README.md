# VFPR - a Vulkan Forward Plus Renderer

**A final project for University of Pennsylvania, CIS 565: GPU Programming and Architecture**

Forward+ (tiled forward) rendering program in Vulkan using compute shader to do light culling. This implementation is __~700%__ faster than regular forward renderer (also in Vulkan) under the condition of 200 lights.

Members:
* [Ruoyu Fan](http://windy.moe/portfolio)
* [Xueyin Wan](https://github.com/xueyinw)

TODO: video

| Render           | Heatmap          |
|------------------|------------------|
| ![](screenshots/render.jpg) | ![](screenshots/heatmap.jpg) |

| Heatmap Only     | Depth Pre-Pass Result |
|------------------|------------------|
| ![](screenshots/heatmap2.jpg) | ![](screenshots/depth.jpg) |

## Description

TODO

#### Frame Breakdown
![](documents/frame_breakdown.png)

![](documents/renderdoc_frame_breakdown.jpg)

TODO

#### Controls

```
Pressing RMB and move cursor: rotate camera
W, S, A, D, Q, E: move camera
Z: toggle debug view
```

## Milestones

### Milestone 3 (12/12/2016)
* Forward+ pipeline:
  * Compute pipeline
  * Shared descriptor sets for lights and camera to use compute pipeline output as fragment shader input.
    * Also used some push constants for the change of tile numbers after resizing window 
  * Tile frustum light culling
  * Depth pre-pass (depth pre-pass + depth culling + light culling all done!)
  * Switch between debug views (HeatMap, Normals, Depth map, etc.)
 
### Milestone 2 (11/28/2016)
* Forward+ framework in progress:
  * Camera control
  * Huge amount of Point lights
  * Normal mapping
  * Blinn-Phong Shading

### Milestone 1 (11/21/2016)
* Basic Vulkan framework including:
  * basic forward rendering pipeline (adaptd from [Vulkan Tutorial by Alexander Overvoorde](https://vulkan-tutorial.com))
  * mesh
  * texture mapping
  * lambert shading
* Cross Platform using GLFW & CMake (Windows, Linux) 

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
* [Correct Frustum Culling](http://www.iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm)
* [Forward+: Bringing Deferred Rendering to the Next Level](https://takahiroharada.files.wordpress.com/2015/04/forward_plus.pdf)
    * Takahiro Harada, Jay McKee, and Jason C. Yang
* [EUROGRAPHICS 2012 Slide Deck](http://www.slideshare.net/takahiroharada/forward-34779335)

#### Libraries
* [CMake](https://cmake.org/)
* [GLFW](http://www.glfw.org/)
* [GLM](http://glm.g-truc.net/0.9.8/index.html)
* [stb_image](https://github.com/nothings/stb)
* [tinyobjloader](https://github.com/syoyo/tinyobjloader)

#### Assets
* [Chalet Hippolyte Chassande Baroz by Gaël](https://skfb.ly/HDVU)
* [Sponza modeled by Marko Dabrovic](http://hdri.cgtechniques.com/~sponza/files/Sponza) (CIS565 simplified version) 

#### Tools
* [RenderDoc](https://github.com/baldurk/renderdoc)  (debugging became a lot easier)
