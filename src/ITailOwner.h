#pragma once
#include <deque>
#include <raylib.h>

// Minimal interface for anything that exposes a tail and basic head parameters.
// Implemented by players and later by enemies.
//
// Virtual functions provide runtime polymorphism - GameManager holds ITailOwner*
// pointers and can update/draw/kill owners without knowing concrete types.
class ITailOwner
{
public:
	virtual ~ITailOwner() = default;

	// Basic data access
	virtual Vector2 GetPosition() const = 0;                    // top-left of head
	virtual Vector2 GetSize() const = 0;                        // head size
	virtual const std::deque<Vector2>& GetTailSegments() const = 0;
	virtual bool IsAlive() const = 0;

	// Lifecycle/update hooks so managers can call uniformly:
	virtual void OwnerUpdate(float deltaTime) = 0; // Update (input + position) per-frame
	virtual void OwnerDraw() = 0;                  // Draw owner (tail + head)
	virtual void OwnerKill() = 0;                  // Kill the owner (set dead)

	// Expose tail "pixel length" so managers can measure/score players (default 0)
	virtual float GetTailPixelLength() const { return 0.0f; }
};