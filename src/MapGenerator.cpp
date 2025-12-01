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
// Draws all obstacles
void MapGenerator::Draw(Color gridColor, bool isPulsating)
{
	int screenWidth_ = GetScreenWidth();
	int screenHeight_ = GetScreenHeight();
	// Draw the Dynamic Floor Grid
	// Pulse alpha between 0.05 and 0.15 for a "breathing" effect
	Color finalColor = gridColor;

	if (isPulsating)
	{
		// CHAOS PULSE: Override the alpha of the passed color
		float time = (float)GetTime();
		float pulse = (sinf(time * 3.0f) * 0.5f + 0.5f) * 0.30f + 0.10f;

		// Apply pulse alpha to the base color
		finalColor = Fade(gridColor, pulse);
	}

	int step = 40; // Grid cell size

	// Vertical Lines
	for (int x = 0; x <= screenWidth_; x += step)
	{
		DrawLine(x, 0, x, screenHeight_, finalColor);
	}

	// Horizontal Lines
	for (int y = 0; y <= screenHeight_; y += step)
	{
		DrawLine(0, y, screenWidth_, y, finalColor);
	}

	// Draw Obstacles with Neon Outline
	for (const auto& wall : obstacles_)
	{
		DrawRectangleRec({ wall.x - 2, wall.y - 2, wall.width + 4, wall.height + 4 }, Fade(DARKBLUE, 0.3f)); // Glow
		DrawRectangleRec(wall, BLACK); // Void center
		DrawRectangleLinesEx(wall, 1.0f, BLUE); // Hard edge
	}
}

// Returns the list of obstacles
const std::vector<Rectangle>& MapGenerator::GetObstacles() const
{
	return obstacles_;
}

// Main map generation function
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
		break;
	case MapType::Fixed:
		GenerateFixedMap(screenWidth, screenHeight);
		break;
	case MapType::Wave:
		GenerateWaveMap(screenWidth, screenHeight);
		break;
	case MapType::Donut:
		GenerateDonutMap(screenWidth, screenHeight);
		break;
	case MapType::Grid:
		GenerateGridMap(screenWidth, screenHeight);
		break;
	case MapType::Chaos:
		GenerateChaosMap(screenWidth, screenHeight);
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

	//considering adding more  blocks spawning on hard mode
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

	// Horizontal Waves (Top and Bottom)
	int numHBlocks = GetRandomValue(20, 30);
	float stepX = (float)screenW / (float)numHBlocks;

	// Tweak properties for horizontal waves
	float ampH = (float)GetRandomValue(40, 80);
	float freqH = (float)GetRandomValue(5, 10) / 10.0f;
	float phaseH = (float)GetRandomValue(0, 360) * DEG2RAD;

	// Base Y positions (Top area and Bottom area)
	Vector2D topBase = { 0.0f, (float)screenH * 0.20f };
	Vector2D botBase = { 0.0f, (float)screenH * 0.80f };

	for (int i = 0; i < numHBlocks; i++)
	{
		float currentX = i * stepX;
		float angle = (i * freqH) + phaseH;

		Vector2D topCenter = { currentX, topBase.y };
		Vector2D botCenter = { currentX, botBase.y };

		// Use the new SineMovement for vertical wiggle
		Vector2D topPos = Vector2D::SineMovement(topCenter, ampH, angle);
		Vector2D botPos = Vector2D::SineMovement(botCenter, ampH, -angle); // Invert phase

		// Add to obstacles
		obstacles_.push_back({ topPos.x, topPos.y, 25.0f, 25.0f });
		obstacles_.push_back({ botPos.x, botPos.y, 25.0f, 25.0f });
	}
}

// Donut map with a large central block
void MapGenerator::GenerateDonutMap(int screenW, int screenH)
{
	// Calculate center and size
	float midX = (float)screenW / 2.0f;
	float midY = (float)screenH / 2.0f;
	float size = 500.0f;

	// Spawns A large central block
	obstacles_.push_back({ midX - (size / 2), midY - (size / 2), size, size });
}

// Grid map made of boxes
void MapGenerator::GenerateGridMap(int screenW, int screenH)
{
	int rows = 4;
	int cols = 4;

	// Define margins so blocks don't spawn too close to the edge walls
	float marginX = screenW * 0.20f;
	float marginY = screenH * 0.15f;

	// Calculate area available for the grid
	float areaW = screenW - (marginX * 2);
	float areaH = screenH - (marginY * 2);

	// Distribute spread
	float stepX = areaW / (cols - 1);
	float stepY = areaH / (rows - 1);

	float boxSize = 120.0f; // Moderately sized obstacles

	for (int y = 0; y < rows; y++)
	{
		for (int x = 0; x < cols; x++)
		{
			// Calculate center positions
			float posX = marginX + (x * stepX);
			float posY = marginY + (y * stepY);

			// Add the box centered at calculation
			obstacles_.push_back({ posX - (boxSize / 2), posY - (boxSize / 2), boxSize, boxSize });
		}
	}
}

// Chaos map with random obstacles
void MapGenerator::GenerateChaosMap(int screenW, int screenH)
{
	// Randomize Wall Count
	int count = GetRandomValue(10, 40);

	for (int i = 0; i < count; i++)
	{
		// Randomize Size
		float w = (float)GetRandomValue(30, 150);
		float h = (float)GetRandomValue(30, 150);

		// Randomize Position
		// Keep a 100px buffer from edge to prevent instant death on spawn
		float x = (float)GetRandomValue(100, screenW - 100 - (int)w);
		float y = (float)GetRandomValue(100, screenH - 100 - (int)h);

		// Define new wall and safe zone rectangles to avoid spawning in center
		Rectangle newWall = { x, y, w, h };
		Rectangle safeZone = { (float)screenW / 2 - 200, (float)screenH / 2 - 200, 400, 400 };

		if (!CheckCollisionRecs(newWall, safeZone))
		{
			obstacles_.push_back(newWall);
		}
	}
}