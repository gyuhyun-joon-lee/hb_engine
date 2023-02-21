#rm -f *.air 2> /dev/null
#rm -f *.metallib 2> /dev/null
#rm -f *.metallibsym 2> /dev/null

# NOTE(gh) type xcrun metal --help to see every flag

# NOTE(gh) We can make .metallib file straight up, but xcode will fail to load our metallibsym file.
# create .air
xcrun -sdk macosx metal -c -gline-tables-only -frecord-sources shader.metal -o build_shader/shader.air
xcrun -sdk macosx metal -c -gline-tables-only -frecord-sources singlepass_shader.metal -o build_shader/singlepass_shader.air
xcrun -sdk macosx metal -c -gline-tables-only -frecord-sources forward_shader.metal -o build_shader/forward_shader.air
xcrun -sdk macosx metal -c -gline-tables-only -frecord-sources grass_shader.metal -o build_shader/grass_shader.air

# create .metallib 
xcrun -sdk macosx metal -gline-tables-only -frecord-sources -o build_shader/shader.metallib build_shader/shader.air build_shader/singlepass_shader.air build_shader/forward_shader.air build_shader/grass_shader.air

# create .metallibsym
xcrun -sdk macosx metal-dsymutil -flat -remove-source build_shader/shader.metallib

#/Volumes/meka/meka_renderer/external/vulkan/macOS/bin/glslc shader.vert -o shader.vert.spv
#/Volumes/meka/meka_renderer/external/vulkan/macOS/bin/glslc shader.frag -o shader.frag.spv

