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
4096rays/pixel, 104.837008ns/ray
![Sample 4](https://github.com/meka-lopo/meka_renderer/blob/3b7d13e3a3aeed71930319cd2c23be8bb024ff03/sample_images/4.png)
256rays/pixel, 104.837008ns/ray
![Sample 5](https://github.com/meka-lopo/meka_renderer/blob/3b7d13e3a3aeed71930319cd2c23be8bb024ff03/sample_images/5.png)
![Sample 1](https://github.com/meka-lopo/meka_renderer/blob/3b7d13e3a3aeed71930319cd2c23be8bb024ff03/sample_images/1.png)
![Sample 2](https://github.com/meka-lopo/meka_renderer/blob/3b7d13e3a3aeed71930319cd2c23be8bb024ff03/sample_images/2.png)
![Sample 3](https://github.com/meka-lopo/meka_renderer/blob/3b7d13e3a3aeed71930319cd2c23be8bb024ff03/sample_images/3.png)


