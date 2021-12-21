# Meka Renderer
Meka Renderer is a file only graphics renderer with minimum integration effort. 
For now, Meka Renderer supports Vulkan on Windows & Metal on MacOS(apple silicion)

# Why?
Third party libraries are generally used to save you from the unwanted works, so that you can focus on what you really want to do. 
However, a lot of the times, I found out that by using the libraries, people(including myself) tend to do more guessing than actually knowing how the code works.

Meka Renderer will be built and designed in a way that nothing is hidden, so that we can collect all the hidden or lost knowledge of how CPU and GPU works, 
which I believe leads to a much better programmer.

Occasionally, I might use some of the single-header libraries such as 'stb_image', but they are for validation purpose only, and will not be included in the final product.

# Caveats
Fow now, Meka Renderer is _not_ meant to be used professionally in any means. This is more like a playground for me to learn and explore.
Feel free to explore, and if you want to keep track of the progress, you can visit _mekalopo.com_

# Sample Images
Config : 8 thread(s), 4 lane(s), 3840px*2160px, 8192 rays/pixel
Raytracing finished in : 217.437683sec
Total Ray Count : 67947724800, Bounced Ray Count : 145755594614, 1.491796ns/ray
![Sample 6](sample_images/6.bmp)

Config : 8 thread(s), 4 lane(s), 1920px*1080px, 2048 rays/pixel
Raytracing finished in : 13.601717sec
Total Ray Count : 4215275520, Bounced Ray Count : 9064849427, 1.500490ns/ray
![Sample 4](sample_images/4.bmp)

Config : 8 thread(s), 4 lane(s), 1920px*1080px, 256 rays/pixel
Raytracing finished in : 1.728225sec
Total Ray Count : 526909440, Bounced Ray Count : 1133122329, 1.525189ns/ray
![Sample 5](sample_images/5.bmp)

![Sample 1](https://github.com/meka-lopo/meka_renderer/blob/3b7d13e3a3aeed71930319cd2c23be8bb024ff03/sample_images/1.png)
![Sample 2](https://github.com/meka-lopo/meka_renderer/blob/3b7d13e3a3aeed71930319cd2c23be8bb024ff03/sample_images/2.png)
![Sample 3](https://github.com/meka-lopo/meka_renderer/blob/3b7d13e3a3aeed71930319cd2c23be8bb024ff03/sample_images/3.png)


