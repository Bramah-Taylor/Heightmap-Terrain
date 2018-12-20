# Heightmap Terrains

![An example of an island output using the application this code is taken from](https://github.com/Bramah-Taylor/Heightmap-Terrain/blob/master/exampleoutput.png "An example of an island output using the application this code is taken from")

The code provided in this repository is designed to demonstrate C++ implementations of several procedural generation algorithms (listed below) for use in creating a heightmap, and applying that heightmap to a plane mesh on the CPU. This application uses DirectX 11 and DirectXTK, in addition to a lecturer-provided framework as part of coursework. The code provided here was originally intended to fulfil requirements of a coursework brief.

The algorithms demonstrated here are as follows:

- Improved Perlin noise (adapted from [Ken Perlin's Java implementation](http://mrl.nyu.edu/~perlin/noise/))
- Simplex noise (adapted from [Stefan Gustavson's Java implementation](http://weber.itn.liu.se/~stegu/simplexnoise/SimplexNoise.java))
- Fractional Brownian motion
- Ridged turbulence
- Thermal erosion
- Hydraulic erosion

## Requirements

The procedural generation algorithms implemented in this repository should be easily portable to other frameworks and applications as they are not dependent on DirectX libraries or the custom framework. Initial functionality is included in the terrain mesh class to demonstrate how a heightmap can be translated into a DirectX mesh (vertex and index buffers) for use in DirectX applications.
