#pragma once
#include <string>
#include <iostream>
#include <deque>
#include "Vector2D.h"
#include <raylib.h>
#include "ITailOwner.h"
#include "PowerUp.h"

class mPlayer : public ITailOwner
{
public:
	// Constructor accepts optional per-instance keys
	mPlayer(Vector2 pStartPos, Vector2 pSize, float pSpeed, float pRotationSpeed, Color pColor, int pKeyLeft = KEY_LEFT, int pKeyRight = KEY_RIGHT);

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

	// Other publicly available utilities
	void CutTrail(size_t cutIndex);

	// Continuous growth: pixels added to tail length per second
	void SetTailGrowthRate(float pixelsPerSecond);
	void SetTailGrowthEnabled(bool enabled);

	// Reset player to a fresh starting state (position, keys optional).
	// pStartAngle is degrees where 0 = right, 90 = down, 180 = left, etc.
	void Reset(Vector2 pStartPos, int pKeyLeft = KEY_LEFT, int pKeyRight = KEY_RIGHT, float pStartAngle = 0.0f);

	void SetControlKeys(int leftKey, int rightKey);

	// Power-up application
	void ApplyPowerUp(PowerUpType type);

	// Power-up status checks
	bool IsPhasing() const;     // For Ghost Mode (ignores collisions)
	bool HasTailCutter() const; // For Cutter Mode
	void ConsumeTailCutter();   // Uses up the cutter charge
	void ClearTail();

	// Simple rectangle of the player's head (for collision checks)
	Rectangle GetRect() const;

private:
	// Direction angle in degrees (0 = right, 90 = down, etc.)
	float directionAngle_;
	Vector2 position_;
	Vector2 size_;
	float speed_;
	float baseSpeed_;
	float rotationSpeed_;
	Color color_;
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
