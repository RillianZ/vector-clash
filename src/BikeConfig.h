#pragma once
#include "raylib.h"

// Different bike types with unique stats
enum class BikeType
{
    Standard,
    Viper,    // Fast, agile, fragile
    Bruiser,  // Slow, , huge
    Ghost,    // Balanced, special ability focus
    Count     // Helper for cycling
};

// The stats container
struct BikeConfig
{
    float speed;
    float turnSpeed; // Degrees per second
    float size;      // Hitbox radius
    float tailGrowthRate;
    Color color;

    // Default Constructor
    BikeConfig()
        : speed(200.0f), turnSpeed(180.0f), size(16.0f), tailGrowthRate(1.0f), color(GRAY)  { }

    // Param Constructor
    BikeConfig(float s, float ts, float sz, float tgr, Color c)
        : speed(s), turnSpeed(ts), size(sz), tailGrowthRate(tgr), color(c) {  }
};

// Static Helper: Centralizes the balancing logic.
// Changing a number here changes it everywhere in the game.
inline BikeConfig GetBikeStats(BikeType type)
{
    switch (type)
    {
    case BikeType::Viper:
        // Fast, very sharp turning, small body, grows tail slowly
        return BikeConfig(300.0f, 190.0f, 12.0f, 0.8f, LIME);

    case BikeType::Bruiser:
        // Slow, but turns fast, huge body, grows tail fast 
        return BikeConfig(150.0f, 220.0f, 24.0f, 2.0f, VIOLET);

    case BikeType::Ghost:
		// Faster speed, faster turn stats, grows tail slowly
        return BikeConfig(260.0f, 200.0f, 16.0f, 0.2f, SKYBLUE);

    case BikeType::Standard:
    default:
        // The baseline functionality
        return BikeConfig(200.0f, 180.0f, 16.0f, 1.0f, RED);
    }
}