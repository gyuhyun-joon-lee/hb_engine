#rm -f *.air 2> /dev/null
#rm -f *.metallib 2> /dev/null
#rm -f *.metallibsym 2> /dev/null

# NOTE(joon): We can make .metallib file straight up, but xcode will fail to load our metallibsym file.
# create .air
xcrun -sdk macosx metal -c -gline-tables-only -frecord-sources shader.metal

# create .metallib 
xcrun -sdk macosx metal -gline-tables-only -frecord-sources -o shader.metallib shader.air

# create .metallibsym
xcrun -sdk macosx metal-dsymutil -flat -remove-source shader.metallib

#/Volumes/meka/meka_renderer/external/vulkan/macOS/bin/glslc shader.vert -o shader.vert.spv
#/Volumes/meka/meka_renderer/external/vulkan/macOS/bin/glslc shader.frag -o shader.frag.spv

