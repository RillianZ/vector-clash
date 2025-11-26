#pragma once
#include <string>
#include <iostream>
#include <vector>
#include "Vector2D.h"
#include "mPlayer.h"
#include <raylib.h>
#include <deque>

// Forward declaration to store a pointer
class mPlayer;
// Enemy class derived from ITailOwner
class Enemy : public ITailOwner
{
public:
	// Constructor
	Enemy(Vector2 startPos, Vector2 size, float speed, float turnSpeed, Color color, int screenW, int screenH);

	// New: Pass the player so the enemy can see the player's tail
	void SetPlayerReference(mPlayer* playerRef);

	// Set reference to obstacles for collision detection
	void SetObstaclesReference(const std::vector<Rectangle>* obstaclesRef);

	//  ITailOwner Interface
	virtual void OwnerUpdate(float dt) override;
	virtual void OwnerDraw() override;
	virtual void OwnerKill() override;
	virtual bool IsAlive() const override;

	virtual Vector2 GetPosition() const override;
	virtual Vector2 GetSize() const override;
	virtual const std::deque<Vector2>& GetTailSegments() const override;
	virtual float GetTailPixelLength() const override;

	// Reset enemy to initial state
	void Reset(Vector2 startPos);

	//  Enemy Specific Methods
	void SetTarget(Vector2 targetPos);

	// Tail length control (pixels)
	void SetCurrentTailLength(float length);

	//  Clear tail segments
	void ClearTail();

private:
	//Helper for "Cone" collision detection ahead of enemy
	bool CheckCollisionAhead(Vector2D start, Vector2D dir, float distance);

	//pointer to obstacles for collision detection
	const std::vector<Rectangle>* obstaclesRef_;

	//Getting position and velocity of enemy
	Vector2D position_;
	float currentAngle_; // Degrees
	float turnSpeed_; // // Degrees per second same as player

	// Store tail as Raylib Vector2 because ITailOwner requires it
	std::deque<Vector2> tailSegments_;
	// Pointer to player for tail checks
	mPlayer* playerRef_;

	int screenWidth_;
	int screenHeight_;

	Vector2D currentTarget_;
	float speed_;
	Color color_;
	Vector2 size_;
	bool isAlive_;

	// Tail length control (pixels)
	float tailLengthPixels_;
	float trailSampleDistance_;
	size_t maxTrailPoints_;
	float growthRatePixelsPerSecond_;
};
