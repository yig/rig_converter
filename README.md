# Rigged Mesh Converter

This is a standalone program to convert rigged models into simple, easy-to-parse formats.
The only dependency is [ASSIMP](https://github.com/assimp/assimp).

## Install [ASSIMP](https://github.com/assimp/assimp)

On a Mac: `brew install assimp`

## Compile

On a Mac or other Unix platform:

    c++ -std=c++11 converter.cpp -o converter -I/usr/local/include -L/usr/local/lib -lassimp -g -Wall

## Run

Example (`test` refers to [ASSIMP](https://github.com/assimp/assimp)'s test mesh directory):

    ./converter test/models-nonbsd/MD5/Bob.md5mesh Bob.obj

This will save the mesh to `Bob.obj` and the rig as `Bob.tgf` (bones) and `Bob.dmat` (skin weights).
You can use any input and output extensions that [ASSIMP](https://github.com/assimp/assimp) supports.
`TGF` and `DMAT` are pretty trivial formats for the skeleton bones and skin weights: http://libigl.github.io/libigl/file-formats/
You can load them with `libigl::readTGF()` and `libigl::readDMAT()` or write your own parser.

Note that the skin weights will be saved flattened, one per each vertex of each face,
rather than one per vertex. This may change in a future update.

NOTE: If you want triangulated output, change the second parameter to `aiImportFile` from `0` to `aiProcess_Triangulate`. You can also use ASSIMP's built-in `assimp` command-line program to convert the mesh (and only use the rig from this project): `assimp export input.whatever output.whatever -tri`.

## License

Public Domain [CC0](http://creativecommons.org/publicdomain/zero/1.0/)
