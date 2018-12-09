#include "TerrainMesh.h"
#include <cmath>
#include <cstdlib>
#include <ctime>

// Initialise buffer and load texture.
TerrainMesh::TerrainMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext, ImprovedNoise* perlinNoise, SimplexNoise* simplexNoise, int lresolution)
{

	int index;
	resolution = lresolution;

	// Create the structure to hold the terrain data.
	heightMap = new HeightMapType[resolution * resolution];

	// Initialise the data in the height map (flat).
	for (int j = 0; j<resolution; j++)
	{

		for (int i = 0; i<resolution; i++)
		{

			index = (resolution * j) + i;

			heightMap[index].x = (float)i / (0.01 * resolution);
			heightMap[index].y = 0.0f;
			heightMap[index].z = (float)j / (0.01 * resolution);

		}

	}

	perlinNoiseGen = perlinNoise;
	simplexNoiseGen = simplexNoise;

	initBuffers(device);

	srand(time(NULL));

}

// Release resources.
TerrainMesh::~TerrainMesh()
{
	// Run parent deconstructor
	BaseMesh::~BaseMesh();
}

void TerrainMesh::GenerateHeightMap(float offsetX, float offsetZ, float frequency, float amplitude, bool ridged, bool simplex, 
	int octaves, float persistence, float offsetY)
{

	int index;	// Index of current vertex

	for (int j = 0; j < resolution; j++)
	{

		for (int i = 0; i < resolution; i++)
		{

			index = (resolution * j) + i;				// Calculate current vertex's position

			heightMap[index].y = offsetY;

			// Initialise values for fractional Brownian motion
			float value = 0.0f;
			float noise = 0.0f;
			float noise2 = 0.0f;						// Second noise value for ridged terrain
			float x = (heightMap[index].x + offsetX);
			float z = (heightMap[index].z + offsetZ);
			float amplitudeLoop = amplitude;
			float frequencyLoop = frequency;

			// Loop for the number of octaves, running the noise function as many times as desired (8 is usually sufficient)
			for (int k = 0; k < octaves; k++)
			{

				// Check whether we're using simplex noise or improved Perlin noise
				if (simplex == true)
				{

					// Perform the noise function
					noise = (simplexNoiseGen->noise(x * frequencyLoop, 0.0f, z * frequencyLoop) * amplitudeLoop);

					// Simple algorithm for generating ridged multifractals
					if (ridged == true)
					{

						// Get a second noise value
						noise2 = (simplexNoiseGen->noise(x * frequencyLoop, 150.0, z * frequencyLoop) * amplitudeLoop);

						// If the new value is greater than the old value, use this instead
						// This will give us valleys
						if (noise2 > noise)
						{

							noise = noise2;

						}
						
						// Invert valleys to give us ridges by using the inverted absolute value
						noise = fabs(noise);
						noise *= -1.0f;

					}

				}
				// Do the same, but using improved Perlin noise
				else
				{

					noise = (perlinNoiseGen->noise(x * frequencyLoop, 0.0, z * frequencyLoop) * amplitudeLoop);

					if (ridged == true)
					{

						noise2 = (perlinNoiseGen->noise(x * frequencyLoop, 150.0, z * frequencyLoop) * amplitudeLoop);

						if (noise2 > noise)
						{

							noise = noise2;

						}
						
						noise = fabs(noise);
						noise *= -1.0f;

					}

				}

				// Add the noise value to the total value
				value += noise;

				// Calculate a new amplitude based on the input persistence/gain value
				// amplitudeLoop will get smaller as the number of layers (i.e. k) increases
				amplitudeLoop *= persistence;
				// Calculate a new frequency based on a lacunarity value of 2.0
				// This gives us 2^k as the frequency
				// i.e. Frequency at k = 4 will be f * 2^4 as we have looped 4 times
				frequencyLoop *= 2.0f;

			}

			// Update the vertex's height
			heightMap[index].y += value;

		}

	}

}

void TerrainMesh::SmoothingFunction(float smoothingWeight, float upperBound, float lowerBound)
{

	// Working values
	int index;									// Index of current vertex
	float v1, v2, v3, v4, v5, v6, v7, v8, v9;	// Current vertex v2 and its Moore neighbourhood

	// Loop vertically
	for (int j = 0; j < resolution; j++)
	{

		// Loop horizontally
		for (int i = 0; i < resolution; i++)
		{

			// Calculate the position of the vertex we need to access within the heightmap
			index = (resolution * j) + i;

			v2 = heightMap[(resolution * j) + i].y;

			if (heightMap[index].y < upperBound && heightMap[index].y > lowerBound)
			{

				// First check corners for smoothing so that we don't go off the edge of the heightmap
				if (i == 0 && j == 0)
				{

					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;
					v6 = heightMap[(resolution * (j + 1)) + (i + 1)].y;

					heightMap[index].y = (v2 * (1 - smoothingWeight)) + (((v3 + v5 + v6) / 3.0f) * smoothingWeight);

				}
				else if (i == resolution - 1 && j == resolution - 1)
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v7 = heightMap[(resolution * (j - 1)) + (i - 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;

					heightMap[index].y = (v2 * (1 - smoothingWeight)) + (((v1 + v7 + v8) / 3.0f) * smoothingWeight);

				}
				else if (i == 0 && j == resolution - 1)
				{


					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;
					v9 = heightMap[(resolution * (j - 1)) + (i + 1)].y;

					heightMap[index].y = (v2 * (1 - smoothingWeight)) + (((v3 + v8 + v9) / 3.0f) * smoothingWeight);

				}
				else if (i == resolution - 1 && j == 0)
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v4 = heightMap[(resolution * (j + 1)) + (i - 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;

					heightMap[index].y = (v2 * (1 - smoothingWeight)) + (((v1 + v4 + v5) / 3.0f) * smoothingWeight);

				}
				// Then check the sides for the same reason
				else if (j == 0)
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v4 = heightMap[(resolution * (j + 1)) + (i - 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;
					v6 = heightMap[(resolution * (j + 1)) + (i + 1)].y;

					heightMap[index].y = (v2 * (1 - smoothingWeight)) + (((v1 + v3 + v4 + v5 + v6) / 5.0f) * smoothingWeight);

				}
				else if (j == resolution - 1)
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v7 = heightMap[(resolution * (j - 1)) + (i - 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;
					v9 = heightMap[(resolution * (j - 1)) + (i + 1)].y;

					heightMap[index].y = (v2 * (1 - smoothingWeight)) + (((v1 + v3 + v7 + v8 + v9) / 5.0f) * smoothingWeight);

				}
				else if (i == 0)
				{

					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;
					v6 = heightMap[(resolution * (j + 1)) + (i + 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;
					v9 = heightMap[(resolution * (j - 1)) + (i + 1)].y;

					heightMap[index].y = (v2 * (1 - smoothingWeight)) + (((v3 + v5 + v6 + v8 + v9) / 5.0f) * smoothingWeight);

				}
				else if (i == resolution - 1)
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v4 = heightMap[(resolution * (j + 1)) + (i - 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;
					v7 = heightMap[(resolution * (j - 1)) + (i - 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;

					heightMap[index].y = (v2 * (1 - smoothingWeight)) + (((v1 + v4 + v5 + v7 + v8) / 5.0f) * smoothingWeight);

				}
				// Then finally do normal smoothing for all other vertices
				else
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v4 = heightMap[(resolution * (j + 1)) + (i - 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;
					v6 = heightMap[(resolution * (j + 1)) + (i + 1)].y;
					v7 = heightMap[(resolution * (j - 1)) + (i - 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;
					v9 = heightMap[(resolution * (j - 1)) + (i + 1)].y;

					heightMap[index].y = (v2 * (1 - smoothingWeight)) + (((v1 + v3 + v4 + v5 + v6 + v7 + v8 + v9) / 8.0f) * smoothingWeight);

				}

			}

		}

	}

}

float TerrainMesh::DepositSediment(float c, float maxDiff, float talus, float distance, float totalDiff)
{

	// Make sure that the distance is greater than the talus angle/threshold
	if (distance > talus)
	{

		// If it is, return the deposition value we want
		return c * (maxDiff - talus) * (distance / totalDiff);

	}
	else
	{

		// Else return nothing
		return 0.0f;

	}

}

void TerrainMesh::ThermalErosion(int erosionIterations)
{

	// Initialise working values
	int index;									// Index of the current vertex
	float v1, v2, v3, v4, v5, v6, v7, v8, v9;	// Vertex v2 and its Moore neighbourhood
	float d1, d2, d3, d4, d5, d6, d7, d8;		// Differences in height for each neighbour
	float talus = 4.0f / resolution;			// Calculate a reasonable talus angle
	float c = 0.5f;								// Constant C
	float maxDiff = 0.0f;
	float totalDiff = 0.0f;

	// Loop for desired iterations
	for (int k = 0; k < erosionIterations; k++)
	{

		// Loop vertically
		for (int j = 0; j < resolution; j++)
		{

			// Loop horizontally
			for (int i = 0; i < resolution; i++)
			{

				// Re-initialise variables for each loop
				maxDiff = 0.0f;
				totalDiff = 0.0f;

				// Initialise height difference variables
				d1 = 0.0f;
				d2 = 0.0f;
				d3 = 0.0f;
				d4 = 0.0f;
				d5 = 0.0f;
				d6 = 0.0f;
				d7 = 0.0f;
				d8 = 0.0f;

				// Calculate the position of the vertex we need to access within the heightmap
				index = (resolution * j) + i;

				// Get ith vertex (h)
				v2 = heightMap[(resolution * j) + i].y;

				// Get the relevant vertices and distances for each ith vertex (need to do this to prevent read access violations)
				if (i == 0 && j == 0)
				{

					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;
					v6 = heightMap[(resolution * (j + 1)) + (i + 1)].y;

					d2 = v2 - v3;
					d4 = v2 - v5;
					d5 = v2 - v6;

				}
				else if (i == resolution - 1 && j == resolution - 1)
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v7 = heightMap[(resolution * (j - 1)) + (i - 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;

					d1 = v2 - v1;
					d6 = v2 - v7;
					d7 = v2 - v8;

				}
				else if (i == 0 && j == resolution - 1)
				{

					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;
					v9 = heightMap[(resolution * (j - 1)) + (i + 1)].y;

					d2 = v2 - v3;
					d7 = v2 - v8;
					d8 = v2 - v9;

				}
				else if (i == resolution - 1 && j == 0)
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v4 = heightMap[(resolution * (j + 1)) + (i - 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;

					d1 = v2 - v1;
					d3 = v2 - v4;
					d4 = v2 - v5;

				}
				else if (j == 0)
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v4 = heightMap[(resolution * (j + 1)) + (i - 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;
					v6 = heightMap[(resolution * (j + 1)) + (i + 1)].y;

					d1 = v2 - v1;
					d2 = v2 - v3;
					d3 = v2 - v4;
					d4 = v2 - v5;
					d5 = v2 - v6;

				}
				else if (j == resolution - 1)
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v7 = heightMap[(resolution * (j - 1)) + (i - 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;
					v9 = heightMap[(resolution * (j - 1)) + (i + 1)].y;

					d1 = v2 - v1;
					d2 = v2 - v3;
					d6 = v2 - v7;
					d7 = v2 - v8;
					d8 = v2 - v9;

				}
				else if (i == 0)
				{

					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;
					v6 = heightMap[(resolution * (j + 1)) + (i + 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;
					v9 = heightMap[(resolution * (j - 1)) + (i + 1)].y;

					d2 = v2 - v3;
					d4 = v2 - v5;
					d5 = v2 - v6;
					d7 = v2 - v8;
					d8 = v2 - v9;

				}
				else if (i == resolution - 1)
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v4 = heightMap[(resolution * (j + 1)) + (i - 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;
					v7 = heightMap[(resolution * (j - 1)) + (i - 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;

					d1 = v2 - v1;
					d3 = v2 - v4;
					d4 = v2 - v5;
					d6 = v2 - v7;
					d7 = v2 - v8;

				}
				else
				{

					v1 = heightMap[(resolution * j) + (i - 1)].y;
					v3 = heightMap[(resolution * j) + (i + 1)].y;
					v4 = heightMap[(resolution * (j + 1)) + (i - 1)].y;
					v5 = heightMap[(resolution * (j + 1)) + i].y;
					v6 = heightMap[(resolution * (j + 1)) + (i + 1)].y;
					v7 = heightMap[(resolution * (j - 1)) + (i - 1)].y;
					v8 = heightMap[(resolution * (j - 1)) + i].y;
					v9 = heightMap[(resolution * (j - 1)) + (i + 1)].y;

					// Calculate the differences of the heights between each vertex
					d1 = v2 - v1;
					d2 = v2 - v3;
					d3 = v2 - v4;
					d4 = v2 - v5;
					d5 = v2 - v6;
					d6 = v2 - v7;
					d7 = v2 - v8;
					d8 = v2 - v9;

				}

				// Get the maximum height and total height
				if (d1 > talus) {

					totalDiff += d1;

					if (d1 > maxDiff) {

						maxDiff = d1;

					}

				}
				if (d2 > talus) {

					totalDiff += d2;

					if (d2 > maxDiff) {

						maxDiff = d2;

					}

				}

				if (d3 > talus) {

					totalDiff += d3;

					if (d3 > maxDiff) {

						maxDiff = d3;

					}

				}

				if (d4 > talus) {

					totalDiff += d4;

					if (d4 > maxDiff) {

						maxDiff = d4;

					}

				}

				if (d5 > talus) {

					totalDiff += d5;

					if (d5 > maxDiff) {

						maxDiff = d5;

					}

				}

				if (d6 > talus) {

					totalDiff += d6;

					if (d6 > maxDiff) {

						maxDiff = d6;

					}

				}

				if (d7 > talus) {

					totalDiff += d7;

					if (d7 > maxDiff) {

						maxDiff = d7;

					}

				}

				if (d8 > talus) {

					totalDiff += d8;

					if (d8 > maxDiff) {

						maxDiff = d8;

					}

				}

				// Assign new heigh values for relevant vertices hi

				// First check corners for erosion so that we don't go off the edge of the heightmap
				if (i == 0 && j == 0)
				{

					heightMap[(resolution * j) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d2, totalDiff);
					heightMap[(resolution * (j + 1)) + i].y += DepositSediment(c, maxDiff, talus, d4, totalDiff);
					heightMap[(resolution * (j + 1)) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d5, totalDiff);

				}
				else if (i == resolution - 1 && j == resolution - 1)
				{

					heightMap[(resolution * j) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d1, totalDiff);
					heightMap[(resolution * (j - 1)) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d6, totalDiff);
					heightMap[(resolution * (j - 1)) + i].y += DepositSediment(c, maxDiff, talus, d7, totalDiff);

				}
				else if (i == 0 && j == resolution - 1)
				{

					heightMap[(resolution * j) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d2, totalDiff);
					heightMap[(resolution * (j - 1)) + i].y += DepositSediment(c, maxDiff, talus, d7, totalDiff);
					heightMap[(resolution * (j - 1)) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d8, totalDiff);

				}
				else if (i == resolution - 1 && j == 0)
				{

					heightMap[(resolution * j) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d1, totalDiff);
					heightMap[(resolution * (j + 1)) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d3, totalDiff);
					heightMap[(resolution * (j + 1)) + i].y += DepositSediment(c, maxDiff, talus, d4, totalDiff);

				}
				// Then check the sides for the same reason
				else if (j == 0)
				{

					heightMap[(resolution * j) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d1, totalDiff);
					heightMap[(resolution * j) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d2, totalDiff);
					heightMap[(resolution * (j + 1)) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d3, totalDiff);
					heightMap[(resolution * (j + 1)) + i].y += DepositSediment(c, maxDiff, talus, d4, totalDiff);
					heightMap[(resolution * (j + 1)) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d5, totalDiff);

				}
				else if (j == resolution - 1)
				{

					heightMap[(resolution * j) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d1, totalDiff);
					heightMap[(resolution * j) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d2, totalDiff);
					heightMap[(resolution * (j - 1)) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d6, totalDiff);
					heightMap[(resolution * (j - 1)) + i].y += DepositSediment(c, maxDiff, talus, d7, totalDiff);
					heightMap[(resolution * (j - 1)) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d8, totalDiff);

				}
				else if (i == 0)
				{

					heightMap[(resolution * j) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d2, totalDiff);
					heightMap[(resolution * (j + 1)) + i].y += DepositSediment(c, maxDiff, talus, d4, totalDiff);
					heightMap[(resolution * (j + 1)) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d5, totalDiff);
					heightMap[(resolution * (j - 1)) + i].y += DepositSediment(c, maxDiff, talus, d7, totalDiff);
					heightMap[(resolution * (j - 1)) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d8, totalDiff);

				}
				else if (i == resolution - 1)
				{

					heightMap[(resolution * j) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d1, totalDiff);
					heightMap[(resolution * (j + 1)) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d3, totalDiff);
					heightMap[(resolution * (j + 1)) + i].y += DepositSediment(c, maxDiff, talus, d4, totalDiff);
					heightMap[(resolution * (j - 1)) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d6, totalDiff);
					heightMap[(resolution * (j - 1)) + i].y += DepositSediment(c, maxDiff, talus, d7, totalDiff);

				}
				// Then finally do normal erosion for all other vertices
				else
				{

					heightMap[(resolution * j) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d1, totalDiff);
					heightMap[(resolution * j) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d2, totalDiff);
					heightMap[(resolution * (j + 1)) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d3, totalDiff);
					heightMap[(resolution * (j + 1)) + i].y += DepositSediment(c, maxDiff, talus, d4, totalDiff);
					heightMap[(resolution * (j + 1)) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d5, totalDiff);
					heightMap[(resolution * (j - 1)) + (i - 1)].y += DepositSediment(c, maxDiff, talus, d6, totalDiff);
					heightMap[(resolution * (j - 1)) + i].y += DepositSediment(c, maxDiff, talus, d7, totalDiff);
					heightMap[(resolution * (j - 1)) + (i + 1)].y += DepositSediment(c, maxDiff, talus, d8, totalDiff);

				}

			}

		}

	}

}

void TerrainMesh::HydraulicErosion(float carryingCapacity, float depositionSpeed, int iterations, int drops, float persistence)
{

	// Place droplets across the terrain until the specified number is reached
	// Generally numbers in the low millions work well for this
	for (int drop = 0; drop < drops; drop++)
	{

		// Get random coordinates to drop the water droplet at
		int X = rand() % resolution;
		int Y = rand() % resolution;

		// Initialise working values
		float carryingAmount = 0.0f;	// Amount of sediment that is currently being carried
		float minSlope = 1.15f;			// Minimum value for the slope/height difference
		float p = 1.0f;					// Persistence

		// Limit calculations to only be run on terrain that is above water
		// This will speed up the calculations considerably
		if (heightMap[(resolution * Y) + X].y > 0.0f)
		{

			// Iterate based on user's input
			for (int iter = 0; iter < iterations; iter++)
			{

				// Get the location of the cell and its von Neumann neighbourhood
				float val = heightMap[(resolution * Y) + X].y;
				float left = 10.0f;
				float right = 10.0f;
				float up = 10.0f;
				float down = 10.0f;

				// Get the relevant vertices and distances for each vertex (need to do this to prevent read access violations)
				if (X == 0 && Y == 0)
				{

					right = heightMap[(resolution * Y) + (X + 1)].y;
					up = heightMap[(resolution * (Y + 1)) + X].y;

				}
				else if (X == resolution - 1 && Y == resolution - 1)
				{

					left = heightMap[(resolution * Y) + (X - 1)].y;
					down = heightMap[(resolution * (Y - 1)) + X].y;

				}
				else if (X == 0 && Y == resolution - 1)
				{

					down = heightMap[(resolution * (Y - 1)) + X].y;
					right = heightMap[(resolution * Y) + (X + 1)].y;

				}
				else if (X == resolution - 1 && Y == 0)
				{

					left = heightMap[(resolution * Y) + (X - 1)].y;
					up = heightMap[(resolution * (Y + 1)) + X].y;

				}
				else if (Y == 0)
				{

					left = heightMap[(resolution * Y) + (X - 1)].y;
					right = heightMap[(resolution * Y) + (X + 1)].y;
					up = heightMap[(resolution * (Y + 1)) + X].y;

				}
				else if (Y == resolution - 1)
				{

					left = heightMap[(resolution * Y) + (X - 1)].y;
					right = heightMap[(resolution * Y) + (X + 1)].y;
					down = heightMap[(resolution * (Y - 1)) + X].y;

				}
				else if (X == 0)
				{

					right = heightMap[(resolution * Y) + (X + 1)].y;
					up = heightMap[(resolution * (Y + 1)) + X].y;
					down = heightMap[(resolution * (Y - 1)) + X].y;

				}
				else if (X == resolution - 1)
				{

					left = heightMap[(resolution * Y) + (X - 1)].y;
					up = heightMap[(resolution * (Y + 1)) + X].y;
					down = heightMap[(resolution * (Y - 1)) + X].y;

				}
				else
				{

					left = heightMap[(resolution * Y) + (X - 1)].y;
					right = heightMap[(resolution * Y) + (X + 1)].y;
					up = heightMap[(resolution * (Y + 1)) + X].y;
					down = heightMap[(resolution * (Y - 1)) + X].y;

				}

				// Find the minimum height value among the cell's neighbourhood, and its location
				float minHeight = val;
				int minIndex = (resolution * Y) + X;

				if (left < minHeight)
				{

					minHeight = left;
					minIndex = (resolution * Y) + (X - 1);

				}

				if (right < minHeight)
				{

					minHeight = right;
					minIndex = (resolution * Y) + (X + 1);

				}

				if (up < minHeight)
				{

					minHeight = up;
					minIndex = (resolution * (Y + 1)) + X;

				}

				if (down < minHeight)
				{

					minHeight = down;
					minIndex = (resolution * (Y - 1)) + X;

				}

				// If the lowest neighbor is NOT greater than the current value
				if (minHeight < val) {

					// Deposit or erode
					float slope = min(minSlope, (val - minHeight));
					float valueToSteal = depositionSpeed * slope;

					// If carrying amount is greater than carryingCapacity
					if (carryingAmount > carryingCapacity)
					{

						// Deposit sediment
						carryingAmount -= valueToSteal;
						heightMap[(resolution * Y) + X].y += valueToSteal * persistence;

					}
					else {

						// Else erode the cell
						// Check that we're within carrying capacity
						if (carryingAmount + valueToSteal > carryingCapacity)
						{

							// If not, calculate the amount that's above the carrying capacity and erode by delta
							float delta = carryingAmount + valueToSteal - carryingCapacity;
							carryingAmount += delta;
							heightMap[(resolution * Y) + X].y -= delta * persistence;

						}
						else
						{

							// Else erode by valueToSteal
							carryingAmount += valueToSteal;
							heightMap[(resolution * Y) + X].y -= valueToSteal * persistence;

						}

					}

					// Move to next value
					if (minIndex == (resolution * Y) + (X - 1)) {

						// Left
						X -= 1;

					}

					if (minIndex == (resolution * Y) + (X + 1)) {

						// Right
						X += 1;

					}

					if (minIndex == (resolution * (Y + 1)) + X) {

						// Up
						Y += 1;

					}

					if (minIndex == (resolution * (Y - 1)) + X) {

						// Down
						Y -= 1;

					}

					// Limiting to edge of map
					if (X > resolution - 1) {

						X = resolution;

					}

					if (Y > resolution - 1) {

						Y = resolution;

					}

					if (Y < 0) {

						Y = 0;

					}

					if (X < 0) {

						X = 0;

					}

					// Decrease persistence for next iteration
					p *= persistence;

				}

			}

		}

	}

}

bool TerrainMesh::CalculateNormals()
{

	int i, j, index1, index2, index3, index, count;
	float vertex1[3], vertex2[3], vertex3[3], vector1[3], vector2[3], sum[3], length;
	VectorType* normals;

	// Create a temporary array to hold the un-normalized normal vectors.
	normals = new VectorType[(resolution - 1) * (resolution - 1)];
	if (!normals)
	{

		return false;

	}

	// Go through all the faces in the mesh and calculate their normals.
	for (j = 0; j<(resolution - 1); j++)
	{

		for (i = 0; i<(resolution - 1); i++)
		{

			index1 = (j * resolution) + i;
			index2 = (j * resolution) + (i + 1);
			index3 = ((j + 1) * resolution) + i;

			// Get three vertices from the face.
			vertex1[0] = heightMap[index1].x;
			vertex1[1] = heightMap[index1].y;
			vertex1[2] = heightMap[index1].z;

			vertex2[0] = heightMap[index2].x;
			vertex2[1] = heightMap[index2].y;
			vertex2[2] = heightMap[index2].z;

			vertex3[0] = heightMap[index3].x;
			vertex3[1] = heightMap[index3].y;
			vertex3[2] = heightMap[index3].z;

			// Calculate the two vectors for this face.
			vector1[0] = vertex1[0] - vertex3[0];
			vector1[1] = vertex1[1] - vertex3[1];
			vector1[2] = vertex1[2] - vertex3[2];
			vector2[0] = vertex3[0] - vertex2[0];
			vector2[1] = vertex3[1] - vertex2[1];
			vector2[2] = vertex3[2] - vertex2[2];

			index = (j * (resolution - 1)) + i;

			// Calculate the cross product of those two vectors to get the un-normalized value for this face normal.
			normals[index].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
			normals[index].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
			normals[index].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);

		}

	}

	// Now go through all the vertices and take an average of each face normal 	
	// that the vertex touches to get the averaged normal for that vertex.
	for (j = 0; j<resolution; j++)
	{

		for (i = 0; i<resolution; i++)
		{

			// Initialize the sum.
			sum[0] = 0.0f;
			sum[1] = 0.0f;
			sum[2] = 0.0f;

			// Initialize the count.
			count = 0;

			// Bottom left face.
			if (((i - 1) >= 0) && ((j - 1) >= 0))
			{

				index = ((j - 1) * (resolution - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;

			}

			// Bottom right face.
			if ((i < (resolution - 1)) && ((j - 1) >= 0))
			{

				index = ((j - 1) * (resolution - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;

			}

			// Upper left face.
			if (((i - 1) >= 0) && (j < (resolution - 1)))
			{

				index = (j * (resolution - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;

			}

			// Upper right face.
			if ((i < (resolution - 1)) && (j < (resolution - 1)))
			{

				index = (j * (resolution - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;

			}

			// Take the average of the faces touching this vertex.
			sum[0] = (sum[0] / (float)count);
			sum[1] = (sum[1] / (float)count);
			sum[2] = (sum[2] / (float)count);

			// Calculate the length of this normal.
			length = sqrt((sum[0] * sum[0]) + (sum[1] * sum[1]) + (sum[2] * sum[2]));

			// Get an index to the vertex location in the height map array.
			index = (j * resolution) + i;

			// Normalize the final shared normal for this vertex and store it in the height map array.
			heightMap[index].nx = (sum[0] / length);
			heightMap[index].ny = (sum[1] / length);
			heightMap[index].nz = (sum[2] / length);

		}

	}

	// Release the temporary normals.
	delete[] normals;
	normals = 0;

	return true;

}

// Generate plane (including texture coordinates and normals).
void TerrainMesh::initBuffers(ID3D11Device* device)
{
	VertexType* vertices;
	unsigned long* indices;
	int index, i, j;
	float positionX, positionZ, u, v, increment;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	int index1, index2, index3, index4;

	// Calculate the number of vertices in the terrain mesh.
	vertexCount = (resolution - 1) * (resolution - 1) * 8;

	indexCount = vertexCount;
	vertices = new VertexType[vertexCount];
	indices = new unsigned long[indexCount];

	index = 0;
	// UV coords.
	u = 0;
	v = 0;
	increment = 0.1f;

	for (j = 0; j<(resolution - 1); j++)
	{

		if (j % 2 != 0)
		{

			for (i = 0; i < (resolution - 1); i++)
			{
				index1 = (resolution * j) + i;          // Bottom left.
				index2 = (resolution * j) + (i + 1);      // Bottom right.
				index3 = (resolution * (j + 1)) + i;      // Upper left.
				index4 = (resolution * (j + 1)) + (i + 1);  // Upper right.

				if (i % 2 == 0)
				{

					// Upper left.
					vertices[index].position = XMFLOAT3(heightMap[index3].x, heightMap[index3].y, heightMap[index3].z);
					vertices[index].normal = XMFLOAT3(heightMap[index3].nx, heightMap[index3].ny, heightMap[index3].nz);
					vertices[index].texture = XMFLOAT2(u, v);
					indices[index] = index;
					index++;

					// Bottom left.
					vertices[index].position = XMFLOAT3(heightMap[index1].x, heightMap[index1].y, heightMap[index1].z);
					vertices[index].normal = XMFLOAT3(heightMap[index1].nx, heightMap[index1].ny, heightMap[index1].nz);
					vertices[index].texture = XMFLOAT2(u, v - increment);
					indices[index] = index;
					index++;

					// Bottom right.
					vertices[index].position = XMFLOAT3(heightMap[index2].x, heightMap[index2].y, heightMap[index2].z);
					vertices[index].normal = XMFLOAT3(heightMap[index2].nx, heightMap[index2].ny, heightMap[index2].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v - increment);
					indices[index] = index;
					index++;

					// Upper left.
					vertices[index].position = XMFLOAT3(heightMap[index3].x, heightMap[index3].y, heightMap[index3].z);
					vertices[index].normal = XMFLOAT3(heightMap[index3].nx, heightMap[index3].ny, heightMap[index3].nz);
					vertices[index].texture = XMFLOAT2(u, v);
					indices[index] = index;
					index++;

					// Bottom right.
					vertices[index].position = XMFLOAT3(heightMap[index2].x, heightMap[index2].y, heightMap[index2].z);
					vertices[index].normal = XMFLOAT3(heightMap[index2].nx, heightMap[index2].ny, heightMap[index2].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v - increment);
					indices[index] = index;
					index++;

					// Upper right.
					vertices[index].position = XMFLOAT3(heightMap[index4].x, heightMap[index4].y, heightMap[index4].z);
					vertices[index].normal = XMFLOAT3(heightMap[index4].nx, heightMap[index4].ny, heightMap[index4].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v);
					indices[index] = index;
					index++;

				}
				else
				{

					// Bottom left.
					vertices[index].position = XMFLOAT3(heightMap[index1].x, heightMap[index1].y, heightMap[index1].z);
					vertices[index].normal = XMFLOAT3(heightMap[index1].nx, heightMap[index1].ny, heightMap[index1].nz);
					vertices[index].texture = XMFLOAT2(u, v - increment);
					indices[index] = index;
					index++;

					// Upper right.
					vertices[index].position = XMFLOAT3(heightMap[index4].x, heightMap[index4].y, heightMap[index4].z);
					vertices[index].normal = XMFLOAT3(heightMap[index4].nx, heightMap[index4].ny, heightMap[index4].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v);
					indices[index] = index;
					index++;

					// Upper left.
					vertices[index].position = XMFLOAT3(heightMap[index3].x, heightMap[index3].y, heightMap[index3].z);
					vertices[index].normal = XMFLOAT3(heightMap[index3].nx, heightMap[index3].ny, heightMap[index3].nz);
					vertices[index].texture = XMFLOAT2(u, v);
					indices[index] = index;
					index++;

					// Bottom left.
					vertices[index].position = XMFLOAT3(heightMap[index1].x, heightMap[index1].y, heightMap[index1].z);
					vertices[index].normal = XMFLOAT3(heightMap[index1].nx, heightMap[index1].ny, heightMap[index1].nz);
					vertices[index].texture = XMFLOAT2(u, v - increment);
					indices[index] = index;
					index++;

					// Bottom right.
					vertices[index].position = XMFLOAT3(heightMap[index2].x, heightMap[index2].y, heightMap[index2].z);
					vertices[index].normal = XMFLOAT3(heightMap[index2].nx, heightMap[index2].ny, heightMap[index2].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v - increment);
					indices[index] = index;
					index++;

					// Upper right.
					vertices[index].position = XMFLOAT3(heightMap[index4].x, heightMap[index4].y, heightMap[index4].z);
					vertices[index].normal = XMFLOAT3(heightMap[index4].nx, heightMap[index4].ny, heightMap[index4].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v);
					indices[index] = index;
					index++;

				}

				u += increment;

			}

		}

		else {

			for (i = 0; i < (resolution - 1); i++)
			{
				index1 = (resolution * j) + i;          // Bottom left.
				index2 = (resolution * j) + (i + 1);      // Bottom right.
				index3 = (resolution * (j + 1)) + i;      // Upper left.
				index4 = (resolution * (j + 1)) + (i + 1);  // Upper right.

				if (i % 2 != 0)
				{

					// Upper left.
					vertices[index].position = XMFLOAT3(heightMap[index3].x, heightMap[index3].y, heightMap[index3].z);
					vertices[index].normal = XMFLOAT3(heightMap[index3].nx, heightMap[index3].ny, heightMap[index3].nz);
					vertices[index].texture = XMFLOAT2(u, v);
					indices[index] = index;
					index++;

					// Bottom left.
					vertices[index].position = XMFLOAT3(heightMap[index1].x, heightMap[index1].y, heightMap[index1].z);
					vertices[index].normal = XMFLOAT3(heightMap[index1].nx, heightMap[index1].ny, heightMap[index1].nz);
					vertices[index].texture = XMFLOAT2(u, v - increment);
					indices[index] = index;
					index++;

					// Bottom right.
					vertices[index].position = XMFLOAT3(heightMap[index2].x, heightMap[index2].y, heightMap[index2].z);
					vertices[index].normal = XMFLOAT3(heightMap[index2].nx, heightMap[index2].ny, heightMap[index2].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v - increment);
					indices[index] = index;
					index++;

					// Upper left.
					vertices[index].position = XMFLOAT3(heightMap[index3].x, heightMap[index3].y, heightMap[index3].z);
					vertices[index].normal = XMFLOAT3(heightMap[index3].nx, heightMap[index3].ny, heightMap[index3].nz);
					vertices[index].texture = XMFLOAT2(u, v);
					indices[index] = index;
					index++;

					// Bottom right.
					vertices[index].position = XMFLOAT3(heightMap[index2].x, heightMap[index2].y, heightMap[index2].z);
					vertices[index].normal = XMFLOAT3(heightMap[index2].nx, heightMap[index2].ny, heightMap[index2].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v - increment);
					indices[index] = index;
					index++;

					// Upper right.
					vertices[index].position = XMFLOAT3(heightMap[index4].x, heightMap[index4].y, heightMap[index4].z);
					vertices[index].normal = XMFLOAT3(heightMap[index4].nx, heightMap[index4].ny, heightMap[index4].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v);
					indices[index] = index;
					index++;

				}
				else
				{

					// Bottom left.
					vertices[index].position = XMFLOAT3(heightMap[index1].x, heightMap[index1].y, heightMap[index1].z);
					vertices[index].normal = XMFLOAT3(heightMap[index1].nx, heightMap[index1].ny, heightMap[index1].nz);
					vertices[index].texture = XMFLOAT2(u, v - increment);
					indices[index] = index;
					index++;

					// Upper right.
					vertices[index].position = XMFLOAT3(heightMap[index4].x, heightMap[index4].y, heightMap[index4].z);
					vertices[index].normal = XMFLOAT3(heightMap[index4].nx, heightMap[index4].ny, heightMap[index4].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v);
					indices[index] = index;
					index++;

					// Upper left.
					vertices[index].position = XMFLOAT3(heightMap[index3].x, heightMap[index3].y, heightMap[index3].z);
					vertices[index].normal = XMFLOAT3(heightMap[index3].nx, heightMap[index3].ny, heightMap[index3].nz);
					vertices[index].texture = XMFLOAT2(u, v);
					indices[index] = index;
					index++;

					// Bottom left.
					vertices[index].position = XMFLOAT3(heightMap[index1].x, heightMap[index1].y, heightMap[index1].z);
					vertices[index].normal = XMFLOAT3(heightMap[index1].nx, heightMap[index1].ny, heightMap[index1].nz);
					vertices[index].texture = XMFLOAT2(u, v - increment);
					indices[index] = index;
					index++;

					// Bottom right.
					vertices[index].position = XMFLOAT3(heightMap[index2].x, heightMap[index2].y, heightMap[index2].z);
					vertices[index].normal = XMFLOAT3(heightMap[index2].nx, heightMap[index2].ny, heightMap[index2].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v - increment);
					indices[index] = index;
					index++;

					// Upper right.
					vertices[index].position = XMFLOAT3(heightMap[index4].x, heightMap[index4].y, heightMap[index4].z);
					vertices[index].normal = XMFLOAT3(heightMap[index4].nx, heightMap[index4].ny, heightMap[index4].nz);
					vertices[index].texture = XMFLOAT2(u + increment, v);
					indices[index] = index;
					index++;

				}

				u += increment;

			}

		}

		u = 0;
		v += increment;
	}

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType)* vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;
	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	// Now create the vertex buffer.
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long)* indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;
	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;
	// Create the index buffer.
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	// Release the arrays now that the buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;
	delete[] indices;
	indices = 0;

}

