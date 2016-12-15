# VFPR - a Vulkan Forward Plus Renderer

**A final project for University of Pennsylvania, CIS 565: GPU Programming and Architecture**

Forward+ (tiled forward) rendering program in Vulkan using compute shader to do light culling. This implementation is __~700%__ faster than regular forward renderer (also in Vulkan) under the condition of 200 lights.

**Team Members**
* Ruoyu Fan
 * [Website](http://windy.moe/portfolio)
 * [LinkedIn](https://www.linkedin.com/in/ruoyuf)
 * [Github](https://github.com/WindyDarian)
* Xueyin Wan
 * [Website](http://xueyin.me/)
 * [Linkedin](https://www.linkedin.com/in/xueyin-wan)
 * [Github](https://github.com/xueyinw)
 
# Overview of Forward Plus Technique
Our ideas of this cool renderer are from this amazing paper: [Forward+: Bringing Deferred Lighting to the Next Level](https://takahiroharada.files.wordpress.com/2015/04/forward_plus.pdf). Thanks so much for these incredible authors!

Forward plus actually is an extension to traditional forward rendering. In forward rendering, it normally limits the number of lights to be valued when shading, which also limits the visibility computation. 

Forward plus extends the forward rendering pipeline by adding a light-culling stage before final shading. Basically this pipeline consists of three stages: depth prepass, light culling, and final shading. We will share more about these stages immediately combined with our Vulkan structure. The advantage of this method is that the scene could be rendered with many lights by culling, and storing only lights that contribute to the tile. Definitely a cool technique, right? :)

Now, let us introduces these three stages in the basic forward plus renderer. In our project, since we use Vulkan, we create three command buffers for each single step.

1. Depth Pre-pass

We inplemented this step by creating a pipeline without fragment shader in Vulkan. This enables depth-write and depth test.

![](screenshots/depth.jpg)

As the picture above shows, this will output a depth map. Which could be used as an input for light culling stage.
    
2. Light Culling

3. Light Accumulation and Final Shading



 
TODO: video

| Render           | Heatmap          |
|------------------|------------------|
| ![](screenshots/render.jpg) | ![](screenshots/heatmap.jpg) |

| Heatmap Only     | Depth Pre-Pass Result |
|------------------|------------------|
| ![](screenshots/heatmap2.jpg) | ![](screenshots/depth.jpg) |

# Description

TODO (add more screenshots)

#### Frame Breakdown
![](documents/frame_breakdown.png)

![](documents/renderdoc_frame_breakdown.jpg)

# Demos

TODO()

# Performance Analysis


#### Controls

```
Pressing RMB and move cursor: rotate camera
W, S, A, D, Q, E: move camera
Z: toggle debug view
```

# Milestones

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

# How to Build

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


# Third-Party Credits

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
