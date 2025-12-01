#pragma once
#include <string>
#include <iostream>
#include <deque>
#include "Vector2D.h"
#include <raylib.h>
#include "ITailOwner.h"
#include "PowerUp.h"
#include "BikeConfig.h"

class mPlayer : public ITailOwner
{
public:
	// Constructor accepts optional per-instance keys
	mPlayer(Vector2 pStartPos, BikeConfig config, int pKeyLeft, int pKeyRight, float startAngle, Color idColor);

	void HandleInput(float deltaTime); // Handles player input for rotation
	void Update(float deltaTime); // Updates the player position and trail
	void Draw(); // Draws the player and its trail
	void Kill(); // Kills the player

	// ITailOwner implementation
	bool IsAlive() const override;
	Vector2 GetPosition() const override;
	Vector2 GetSize() const override;

	// Lifecycle hooks (used by GameManager)
	void OwnerUpdate(float deltaTime) override; // calls input+update
	void OwnerDraw() override;                  // draws tail + head
	void OwnerKill() override;                  // calls mPKill()

	// Continuous growth: pixels added to tail length per second
	void SetTailGrowthRate(float pixelsPerSecond);
	void SetTailGrowthEnabled(bool enabled);

	// Reset player to a fresh starting state (position, keys optional).
	// pStartAngle is degrees where 0 = right, 90 = down, 180 = left, etc.
	void Reset(Vector2 pStartPos, BikeType type_, float startAngle);

	// Returns the bike type
	BikeType GetType() const { return type_; }

	// Sets the control keys for the player
	void SetControlKeys(int leftKey, int rightKey);

	// Power-up application
	void ApplyPowerUp(PowerUpType type);

	// Power-up status checks
	bool IsPhasing() const;     // For Ghost Mode (ignores collisions)
	bool HasTailCutter() const; // For Cutter Mode
	void ConsumeTailCutter();   // Uses up the cutter charge
	void ClearTail(); // Clears the tail segments

	// Simple rectangle of the player's head (for collision checks)
	Rectangle GetRect() const;

private:
	// Sets bike type and updates stats accordingly
	BikeType type_;

	// Direction angle in degrees (0 = right, 90 = down, etc.)
	float directionAngle_;

	// physics and rendering properties
	Vector2 position_;
	Vector2 size_;
	float speed_;
	float baseSpeed_;
	float rotationSpeed_;
	Color color_;
	Color identityColor_;
	bool isAlive_;

	// Continuous growth
	bool tailGrowthEnabled_;
	float growthRatePixelsPerSecond_;

	// Per-instance control keys
	int keyLeft_;
	int keyRight_;

	// Power-up states
	float speedTimer_;
	float ghostTimer_;
	bool hasTailCutter_;
};
