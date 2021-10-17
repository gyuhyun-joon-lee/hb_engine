# Meka Renderer
Meka Renderer is a file only graphics renderer with minimum integration effort. 
For now, Meka Renderer only supports Vulkan API on windows, but will be supporting MacOS with other graphic APIs such as OpenGL, Metal, and even possibly without any APIs.

# Why?
Third party libraries are meant to save you from unwanted works, so that you can focus on what you really want to do. 
However, a lot of the times, I find out that by using those libraries, people(including myself) tend to do more guessing than actually knowing how the code works.

Meka Renderer will be built and designed in a way that nothing is hidden, so that we can collect all the hidden or lost knowledges of CPU and GPU, and really understand how each part works and designed.

# How To Use
A good starting point would be using the platform layer(pre fixed with platform name such as win32, macos.. ) that is included in the source code, as that is the code I'm using.
If you want to be more adventurous, you can also just include "meka_vulkan.cpp" inside your main code and make it work.

# Missing Features
When there's light, there's also a shadow. Building a renderer from scratch means the implementation speed will be a lot slower, which means there will be a lot of missing 
features for now. I'll just gonna mention some of those features, but as we go furthur, _many_ will appear more. But we'll get there.

_urgent_
- 3d model drawing
- custom obj/glTF loader, starting with a very slow one.

_not_so_urgent_
- macos support using vulkan(graphics), vulkan(compute), metal, opengl, software renderer
- win32 support using vulkan(compute), metal, opengl, software renderer
