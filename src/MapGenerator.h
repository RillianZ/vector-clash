#pragma once
#include <vector>
#include <raylib.h>
#include "Vector2D.h"

// Map types
enum class MapType
{
	Clean,      // No obstacles (Stage 1)
	Fixed,      // Static, hardcoded obstacles (Stage 2)
	Wave        // Math-generated procedural obstacles (Stage 3)
};

class MapGenerator
{
public:
	// Constructor and Destructor
	MapGenerator();
	~MapGenerator();

	// Main function to build the map based on the selected type
	void GenerateMap(MapType type, int screenWidth, int screenHeight);

	// Draws all active obstacles
	void Draw();

	// Returns the list of obstacles for collision detection
	//uses a const reference to avoid copying the vector
	const std::vector<Rectangle>& GetObstacles() const;

private:
	// The container for all wall segments
	std::vector<Rectangle> obstacles_;

	// Internal helpers to keep the code clean
	void GenerateFixedMap(int screenW, int screenH);
	void GenerateWaveMap(int screenW, int screenH);
};