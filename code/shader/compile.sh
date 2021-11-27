rm -f *.air 2> /dev/null
rm -f *.metallib 2> /dev/null
rm -f *.metallibsym 2> /dev/null

# NOTE(joon): We can make .metallib file straight up, but xcode will fail to load our metallibsym file.
# create .air
xcrun -sdk macosx metal -c -frecord-sources shader.metal

# create .metallib 
xcrun -sdk macosx metal -frecord-sources -o shader.metallib shader.air

# create .metallibsym
xcrun -sdk macosx metal-dsymutil -flat -remove-source shader.metallib

