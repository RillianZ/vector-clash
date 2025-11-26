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

	void mPHandleInput(float deltaTime); // Handles player input for rotation
	void mPUpdate(float deltaTime); // Updates the player position and trail
	void mPDraw(); // Draws the player and its trail
	void mPKill(); // Kills the player

	// ITailOwner implementation
	Vector2 GetPosition() const override;
	Vector2 GetSize() const override;
	const std::deque<Vector2>& GetTailSegments() const override;
	bool IsAlive() const override;
	float GetTailPixelLength() const override;

	// Lifecycle hooks (used by GameManager)
	void OwnerUpdate(float deltaTime) override; // calls input+update
	void OwnerDraw() override;                  // draws tail + head
	void OwnerKill() override;                  // calls mPKill()

	// Other publicly available utilities
	void CutTrail(size_t cutIndex);

	// Tail length control (pixels)
	void SetTailLength(float pixels);

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

	Vector2 m_position;
	Vector2 m_size;
	float m_speed;
	float m_baseSpeed;
	float m_rotationSpeed;
	// Direction angle in degrees (0 = right, 90 = down, etc.)
	float m_directionAngle;
	Color m_color;
	bool m_isAlive;

	// Trail storage (deque so popping front is O(1))
	std::deque<Vector2> m_trail;

	// Trail sampling & length control
	float m_tailLengthPixels;        // total length of tail in pixels
	float m_trailSampleDistance;     // minimum distance between stored trail points (pixels)
	size_t m_maxTrailPoints;         // computed from tailLength / sampleDistance

	// Continuous growth
	bool m_tailGrowthEnabled;
	float m_growthRatePixelsPerSecond;

	// Per-instance control keys
	int m_keyLeft;
	int m_keyRight;

	// Power-up states
	float m_speedTimer;
	float m_ghostTimer;
	bool m_hasTailCutter;
};
