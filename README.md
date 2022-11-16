# HB Engine
HB engine is a 3D game engine based on C++ and Metal with minimum usage of thrid party libraries. 
Currently HB engine uses these libraries, which will be replaced later down the road.
- stb truetype libary(stb_truetype.h)

# Why?
These days, people use a lot of libraries so that they can focus on what they really want to do. However a lot of the times, I found out that by using those libraries, people(including myself) tend to do more guessing than actually knowing how the code works.
Inspired by the Handmade Hero Show by Casey Muratori, HB Engine will be built and designed in a way that nothing is hidden, so that we can fully understand how each part of the game engine works.

# How To
Currently, HB engine only builds and runs on MacOS with apple sillicons, as many of the optimizations using SIMD are based on ARM architecture.
If you have one of the apple sillicon Macs and wanna build the project, please follow these steps : 

1. Make sure you installed the Command Line Tools for Xcode.(This normally comes with the Xcode itself)
2. Navigate to the 'code' directory using the terminal.
3. Type 'make' and press enter. This will automatically build the project using the existing makefile, which builds an executable(app) inside the 'build' directory. 
4. Navigate back to the base directory. You should be able to see the 'build' directory.
5. Inside the build directory, you will see the app called 'HB.app'. 

# Features
For now, HB engine is capable of :
- Rendering 1M procedural grasses using the traditional compute/vertex pipelines, or 260K grasses using mesh shading in 60FPS.
    - Inspired by the game Ghost of Tsushima https://youtu.be/Ibe1JBF5i5Y
- Static & dynamic wind using fluid dynamics
- Live code editing, which allows you to edit game code without closing the application.

# Showcase
Showcase #1(2022/10/12)
Change: Procedural grass rendering using mesh shading(Metal 3)
https://youtu.be/kZ75YueSfdk
![Sample 1](showcase/2022_10_12.png)

Showcase #2(2022/11/16)
Change: Rudimentary dynamic wind using fluid dynamics(optimized with SIMD and multi-threading)
![Sample 2](showcase/2022_11_16.png)



