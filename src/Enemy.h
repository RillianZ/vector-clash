#pragma once
#include <string>
#include <iostream>
#include <vector>
#include "Vector2D.h"
#include "mPlayer.h"
#include <raylib.h>
#include <deque>

class Enemy : public ITailOwner
{
public:
	Enemy(Vector2 startPos, float speed, Color color);

	// --- ITailOwner Interface Implementation ---
	virtual void OwnerUpdate(float dt) override;
	virtual void OwnerDraw() override;
	virtual void OwnerKill() override;
	virtual bool IsAlive() const override;

	virtual Vector2 GetPosition() const override;
	virtual Vector2 GetSize() const override;
	virtual const std::deque<Vector2>& GetTailSegments() const override;
	virtual float GetTailPixelLength() const override;

	// --- Enemy Specific Methods ---
	void SetTarget(Vector2 targetPos);

private:
	// LOGIC: Use your custom class for position/velocity so we can use your math methods
	Vector2D position_;

	// DATA: Store tail as Raylib Vector2 because ITailOwner requires it
	std::deque<Vector2> tailSegments_;

	Vector2D currentTarget_;
	float speed_;
	Color color_;
	Vector2 size_;
	bool isAlive_;

	float timeSinceLastTailDrop_;
	float tailDropInterval_;
};
