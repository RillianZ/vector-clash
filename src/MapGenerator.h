#pragma once
#include <vector>
#include <raylib.h>
#include "Vector2D.h"

// Map types
enum class MapType
{
	Clean,      // No obstacles (Stage 1)
	Fixed,      // 4 static cubes in the middle (Stage 2)
	Wave        // Math-generated  Sin waves (Stage 3)
};

// MapGenerator class handles map creation and obstacle management
class MapGenerator
{
public:
	// Constructor
	MapGenerator();

	// Destructor
	~MapGenerator();

	// Main function to build the map based on the selected type
	void GenerateMap(MapType type, int screenWidth, int screenHeight);

	// Draws all active obstacles
	void Draw();

	// Returns the list of obstacles for collision detection, const to avoid bugs
	const std::vector<Rectangle>& GetObstacles() const;

private:
	// The container for all wall segments
	std::vector<Rectangle> obstacles_;

	// Internal helpers to keep the code clean
	void GenerateFixedMap(int screenW, int screenH);
	void GenerateWaveMap(int screenW, int screenH);
};