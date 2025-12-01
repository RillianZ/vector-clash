#pragma once
#include <deque>
#include <raylib.h>
#include "Vector2D.h"

// Implemented by players and enemies.
// My first proper Virtual function that provide runtime polymorphism, GameManager holds ITailOwner*
// pointers and can update/draw/kill owners without knowing concrete types.
class ITailOwner
{
public:
	// Virtual destructor for proper cleanup of derived classes
	virtual ~ITailOwner() = default;

	// top-left of head
	virtual Vector2 GetPosition() const = 0;

	// head size
	virtual Vector2 GetSize() const = 0;

	// Returns the raw deque of points we stored in protected
	virtual const std::deque<Vector2>& GetTailSegments() const { return tailSegments_; }

	//  Check if owner is alive
	virtual bool IsAlive() const = 0;

	// Update (input + position) per-frame
	virtual void OwnerUpdate(float deltaTime) = 0;

	// Draw owner (tail + head)
	virtual void OwnerDraw() = 0;

	// Kill the owner (set dead)
	virtual void OwnerKill() = 0;

	// Returns total length in pixels
	virtual float GetTailPixelLength() const { return tailLengthPixels_; }

	// Setter for length (Used by PowerUps/Growth)
	void SetTailLength(float pixels)
	{
		// Safety minimum
		if (pixels < 20.0f) pixels = 20.0f;
		tailLengthPixels_ = pixels;
	}

	void CutTailAtIndex(size_t index)
	{
		if (index >= tailSegments_.size()) return;
		//Remove segments
		size_t removeCount = index + 1;
		for (size_t i = 0; i < removeCount && !tailSegments_.empty(); i++)
		{
			tailSegments_.pop_front();
		}
		//Recalculate Length Stats
		tailLengthPixels_ = (float)tailSegments_.size() * trailSampleDistance_;

		//Safety clamp
		if (tailLengthPixels_ < 20.0f) tailLengthPixels_ = 20.0f;
	}

protected:

	// Shared tail data
	std::deque<Vector2> tailSegments_;

	// Tail length in pixels
	float tailLengthPixels_ = 200.0f;

	// distance between tail samples
	float trailSampleDistance_ = 3.0f;

	// max tail segments to store
	size_t maxTrailPoints_ = 0;

	// Shared tail update logic
	void UpdateTailLogic(Vector2 currentPos)
	{
		// Initialize if empty
		if (tailSegments_.empty())
		{
			tailSegments_.push_back(currentPos);
			return;
		}

		// calculate distance from last segment to current position and decide whether to add a new point
		Vector2D lastVec = { tailSegments_.back().x, tailSegments_.back().y };
		Vector2D currVec = { currentPos.x, currentPos.y };

		//Add a new permanent point if enouguh distance traveled
		if (lastVec.DistanceToTarget(currVec) >= trailSampleDistance_)
		{
			tailSegments_.push_back(currentPos);
		}
		else
		{
			// update the very last point to the current position
			tailSegments_.back() = currentPos;
		}

		// Recalculate max points based on current pixel length every frame
		maxTrailPoints_ = static_cast<size_t>(tailLengthPixels_ / trailSampleDistance_);
		if (maxTrailPoints_ < 2) maxTrailPoints_ = 2;

		//Trim old segments
		while (tailSegments_.size() > maxTrailPoints_)
		{
			tailSegments_.pop_front();
		}
	}

	// Helper to clear tail (for resets or cutting)
	void ResetTail(Vector2 startPos)
	{
		tailSegments_.clear();
		tailSegments_.push_back(startPos);
	}
};