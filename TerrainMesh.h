// TerrainMesh.h
// Generates a plane mesh based on resolution, which can be manipulated using a heightmap.
// Uses "quilt" pattern to build a series of quads across the mesh.
// Adapted from a combination of the CMP301 plane mesh, the CMP301 quad mesh, and the Rastertek terrain mesh provided in the terrain generation tutorials

#ifndef _TERRAINMESH_H_
#define _TERRAINMESH_H_

#include "../DXFramework/BaseMesh.h"
#include "ImprovedNoise.h"
#include "SimplexNoise.h"

class TerrainMesh : public BaseMesh
{

	struct HeightMapType
	{

		float x, y, z;
		float nx, ny, nz;

	};

	struct VectorType
	{

		float x, y, z;

	};

public:

	TerrainMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext, ImprovedNoise* perlinNoise, SimplexNoise* simplexNoise, int resolution = 100);
	~TerrainMesh();

	// Function for generating the heightmap using fractional Brownian motion alongside Perlin noise or Simplex noise
	void GenerateHeightMap(float offsetX, float offsetZ, float frequency, float amplitude, bool ridged, bool simplex, 
		int octaves, float persistence, float offsetY);

	// Function for smoothing out generated terrain within given height bounds
	void SmoothingFunction(float smoothingWeight, float upperBound, float lowerBound);

	// Thermal erosion simulates material breaking loose and sliding down slopes over time
	// Results in generally smoother, flatter terrain
	// Reference implementation: http://web.mit.edu/cesium/Public/terrain.pdf
	void ThermalErosion(int erosionIterations);

	// Hydraulic erosion simulates the effects of water on terrain over time by depositing droplets over terrain
	// Results in ridged, rough terrain
	// Reference implementation: https://github.com/vogtb/terrain-map/blob/master/landmap.js
	void HydraulicErosion(float carryingCapacity, float depositionSpeed, int iterations, int drops, float persistence);

	void initBuffers(ID3D11Device* device);

	bool CalculateNormals();

private:

	// Function for depositing sediment from the thermal erosion algorithm
	float DepositSediment(float c, float maxDiff, float talus, float distance, float totalDiff);

	int resolution;
	HeightMapType* heightMap;

	// Pointers to the noise generation objects
	ImprovedNoise* perlinNoiseGen;
	SimplexNoise* simplexNoiseGen;

};

#endif