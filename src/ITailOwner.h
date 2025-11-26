#pragma once
#include <deque>
#include <raylib.h>

// Implemented by players and enemies.
// My first proper Virtual function that provide runtime polymorphism, GameManager holds ITailOwner*
// pointers and can update/draw/kill owners without knowing concrete types.
class ITailOwner
{
public:
	// Virtual destructor for proper cleanup of derived classes
	virtual ~ITailOwner() = default;

	// Basic data access

	// top-left of head
	virtual Vector2 GetPosition() const = 0;

	// head size
	virtual Vector2 GetSize() const = 0;

	// read-only access to tail segments (deque of Vector2)
	virtual const std::deque<Vector2>& GetTailSegments() const = 0;

	//  Check if owner is alive
	virtual bool IsAlive() const = 0;

	// Lifecycle/update hooks so managers can call uniformly:

	// Update (input + position) per-frame
	virtual void OwnerUpdate(float deltaTime) = 0;

	// Draw owner (tail + head)
	virtual void OwnerDraw() = 0;

	// Kill the owner (set dead)
	virtual void OwnerKill() = 0;

	// Expose tail "pixel length" to managers for scoring, etc.
	virtual float GetTailPixelLength() const { return 0.0f; }
};