#include "MapGenerator.h"
#include <iostream> 

// Constructor
MapGenerator::MapGenerator()
{
}
// Destructor
MapGenerator::~MapGenerator()
{
	obstacles_.clear();
}

void MapGenerator::Draw()
{
	for (const auto& wall : obstacles_)
	{
		DrawRectangleRec(wall, GRAY);
		DrawRectangleLinesEx(wall, 1.0f, DARKGRAY); // Adds a border for visibility
	}
}

const std::vector<Rectangle>& MapGenerator::GetObstacles() const
{
	return obstacles_;
}

void MapGenerator::GenerateMap(MapType type, int screenWidth, int screenHeight)
{
	obstacles_.clear();

	// Always adds boundary walls (Top, Bottom, Left, Right)
	int thickness = 10;
	obstacles_.push_back({ 0.0f, 0.0f, (float)screenWidth, (float)thickness }); // Top
	obstacles_.push_back({ 0.0f, (float)screenHeight - thickness, (float)screenWidth, (float)thickness }); // Bottom
	obstacles_.push_back({ 0.0f, 0.0f, (float)thickness, (float)screenHeight }); // Left
	obstacles_.push_back({ (float)screenWidth - thickness, 0.0f, (float)thickness, (float)screenHeight }); // Right

	// map logic
	switch (type)
	{
	case MapType::Clean:
		// Just boundaries
		break;

	case MapType::Fixed:
		GenerateFixedMap(screenWidth, screenHeight);
		break;

	case MapType::Wave:
		GenerateWaveMap(screenWidth, screenHeight);
		break;
	}

	std::cout << "Map Generated. Total Obstacles: " << obstacles_.size() << std::endl;
}
// Fixed map with hardcoded obstacles
void MapGenerator::GenerateFixedMap(int screenW, int screenH)
{
	float midX = (float)screenW / 2.0f;
	float midY = (float)screenH / 2.0f;
	float wallSize = 100.0f;
	float offset = 250.0f;

	// Add 4 static blocks in a square from center
	// Top Left
	obstacles_.push_back({ midX - offset, midY - offset, wallSize, wallSize });
	// Top Right
	obstacles_.push_back({ midX + offset - wallSize, midY - offset, wallSize, wallSize });
	// Bottom Left
	obstacles_.push_back({ midX - offset, midY + offset - wallSize, wallSize, wallSize });
	// Bottom Right
	obstacles_.push_back({ midX + offset - wallSize, midY + offset - wallSize, wallSize, wallSize });
}
	
// Wave map with procedurally generated wavy walls using teacher taught Vector2D class
// Since CosineMovement modifies X based on an angle, we step down Y and calculate X using the class.
void MapGenerator::GenerateWaveMap(int screenW, int screenH)
{
	//Amount of blocks to create
	//tweak these settings to tweak the wave appearance
	int numBlocks = GetRandomValue(12, 18);
	float stepY = (float)screenH / (float)numBlocks;
	float amplitude = (float)GetRandomValue(50, 100); // Wideness of wave now with random value for more spice
	float frequency = (float)GetRandomValue(5, 8) / 10.0f;   // Tightness of wave, also with random value for spice
	float blockSize = 25.0f; // Size of obstacle blocks

	// Random offset from getrandomvalue to vary starting phase
	float phaseShift = (float)GetRandomValue(0, 360) * DEG2RAD;

	//tweak these settings to tweak the wave positioning along the sides (percentage)
	// Left Side Wave
	Vector2D leftBase = { (float)screenW * 0.15f, 0.0f }; 

	// Right Side Wave
	Vector2D rightBase = { (float)screenW * 0.85f, 0.0f }; 

	// Loop to create blocks down the screen
	for (int i = 0; i < numBlocks; i++)
	{
		// Calculate the Y position manually (stepping down)
		float currentY = i * stepY;
		float angle = (i * frequency) + phaseShift;

		// Use Vector2D to calculate the X wobble
		Vector2D leftCenter = { leftBase.x, currentY };
		Vector2D rightCenter = { rightBase.x, currentY };

		//CosineMovement(Center, Amp, Angle)
		Vector2D leftPos = Vector2D::CosineMovement(leftCenter, amplitude, angle);

		// Inverted right side for mirror effect
		Vector2D rightPos = Vector2D::CosineMovement(rightCenter, amplitude, -angle);

		// Add obstacles
		obstacles_.push_back({ leftPos.x, leftPos.y, blockSize, blockSize });
		obstacles_.push_back({ rightPos.x, rightPos.y, blockSize, blockSize });
	}
}