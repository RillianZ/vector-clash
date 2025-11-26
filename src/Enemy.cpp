#include "Enemy.h"
#include <cmath>
#include "mPlayer.h"
#include "raymath.h"

// Helper to normalize angle difference to -180 to 180 range
float NormalizeAngleDiff(float diff)
{
	while (diff > 180.0f) diff -= 360.0f;
	while (diff < -180.0f) diff += 360.0f;
	return diff;
}

// Constructor
Enemy::Enemy(Vector2 startPos, Vector2 size, float speed, float turnSpeed, Color color, int screenW, int screenH)
	: position_({ startPos.x, startPos.y }) // Initialize position
	, turnSpeed_(turnSpeed) // Initialize turn speed
	, speed_(speed) // Initialize speed
	, color_(color) // Initialize color
	, size_(size) // Initialize size
	, isAlive_(true) // Start alive
	, currentTarget_({ 0, 0 }) // Initialize target
	, currentAngle_(0.0f) // Initialize angle
	, playerRef_(nullptr) // Initialize player to nullptr
	, obstaclesRef_(nullptr)  // Initialize obstacle to nullptr
	, screenWidth_(screenW) // Initialize screen width
	, screenHeight_(screenH) // Initialize screen height
	, growthRatePixelsPerSecond_(40.0f) // Grow same speed as player
{
	ResetTail({ position_.x, position_.y });
}

//sets the obstacles reference for collision detection
void Enemy::SetObstaclesReference(const std::vector<Rectangle>* obstaclesRef)
{
	obstaclesRef_ = obstaclesRef;
}

//sets the player reference for tail checks
void Enemy::SetPlayerReference(mPlayer* playerRef)
{
	playerRef_ = playerRef;
}

// Sets the current target position for the enemy to seek
void Enemy::SetTarget(Vector2 targetPos)
{
	currentTarget_ = { targetPos.x, targetPos.y };
}

// Reset enemy to initial state
void Enemy::Reset(Vector2 startPos)
{
	// Reset position
	position_ = { startPos.x, startPos.y };

	// Reset angle (90 = Down)
	currentAngle_ = 90.0f;
	
	// Set alive
	isAlive_ = true;

	// Reset tail length parameters
	tailLengthPixels_ = 200.0f;
	
	// Reset trail history (Parent Helper)
	ResetTail({ position_.x, position_.y });
}

// Clears the tail segments
void Enemy::ClearTail()
{
	ResetTail({ position_.x, position_.y });
}

// The "Enemy Cone Check/Whiskers" function
bool Enemy::CheckCollisionAhead(Vector2D start, Vector2D dir, float lookAheadDist)
{
	//Checking 3 stages ahead for more reliable detection
	int checkSteps = 3;
	float stepDist = lookAheadDist / (float)checkSteps;

	// Iterate through each step along the feeler
	for (int k = 1; k <= checkSteps; k++)
	{
		// Current distance along the feeler
		float currentDist = stepDist * k;

		// Calculate the tip of the feelers using Vector2D math
		Vector2D tip = start.SetVectorOffset(dir.ScaleVector(currentDist));

		//  Wall Checks - Screen Bounds
		// buffer is to avoid getting too close to edge (Tweakable)
		const int buffer = 20;
		if (tip.x < buffer || tip.x > screenWidth_ - buffer || tip.y < buffer || tip.y > screenHeight_ - buffer) return true;

		// Obstacle Checks using obstaclesRef_ pointer to access walls
		if (obstaclesRef_)
		{
			for (const auto& wall : *obstaclesRef_)
			{
				// CheckCollisionPointRec is a Raylib function: (Point, Rectangle)
				// We use the tip of the whisker as the point
				if (CheckCollisionPointRec({ tip.x, tip.y }, wall)) return true;
			}
		}

		//  Self Tail Chec
		size_t skipRecent = 20;
		if (tailSegments_.size() > skipRecent)
		{
			for (size_t i = 0; i < tailSegments_.size() - skipRecent; i += 5)
			{
				Vector2D seg = { tailSegments_[i].x, tailSegments_[i].y };
				// If we are about to hit a tail segment
				if (tip.DistanceToTarget(seg) < size_.x * 1.5f) return true;
			}
		}

		// Player Tail Check
		// uses playerRef_ pointer to access player's tail segments
		if (playerRef_ && playerRef_->IsAlive())
		{
			// Skip recent segments to avoid immediate collision
			const auto& pTail = playerRef_->GetTailSegments();
			for (size_t i = 0; i < pTail.size(); i += 5)
			{
				Vector2D seg = { pTail[i].x, pTail[i].y };
				if (tip.DistanceToTarget(seg) < size_.x * 1.5f) return true;
			}
		}
	}
	return false;
}

// Update (input + position) per-frame
// Most of the enemy logic is here!
void Enemy::OwnerUpdate(float dt)
{
	if (!isAlive_) return;

	// STEERING & MOVEMENT

	//  Calculate Forward Direction
	// Using CircularMotion helper to get direction vector from angle
	Vector2D currentDir = Vector2D::CircularMotion({ 0,0 }, 1.0f, currentAngle_ * DEG2RAD);
	float lookAhead = speed_ * 0.7f;

	// Calculate Whiskers using ROTATION
	float whiskerAngleRad = 0.8f;
	Vector2D leftDir = currentDir.Rotate(-whiskerAngleRad);
	Vector2D rightDir = currentDir.Rotate(whiskerAngleRad);

	//Collision Checks
	bool hitFront = CheckCollisionAhead(position_, currentDir, lookAhead);
	bool hitLeft = CheckCollisionAhead(position_, leftDir, lookAhead * 0.5f);
	bool hitRight = CheckCollisionAhead(position_, rightDir, lookAhead * 0.5f);

	//  Steering Input
	float steeringInput = 0.0f;

	if (hitFront || hitLeft || hitRight)
	{
		if (hitFront)
		{
			if (hitLeft && !hitRight) steeringInput = 1.0f;
			else if (hitRight && !hitLeft) steeringInput = -1.0f;
			else
			{
				if (position_.x > screenWidth_ / 2.0f) steeringInput = -1.0f;
				else steeringInput = 1.0f;
			}
		}
		else if (hitLeft) steeringInput = 1.0f;
		else if (hitRight) steeringInput = -1.0f;
	}
	else
	{
		// Seek Logic
		Vector2D toTarget = position_.VectorTowardsTarget(currentTarget_);

		// Uses GetAngle() instead of manual atan2f
		float targetAngleRad = toTarget.GetAngle();
		float targetAngleDeg = targetAngleRad * RAD2DEG;

		//  Uses static NormalizeAngleDiff
		float diff = Vector2D::NormalizeAngleDiff(targetAngleDeg - currentAngle_);

		if (diff > 5.0f) steeringInput = 1.0f;
		else if (diff < -5.0f) steeringInput = -1.0f;
		else steeringInput = diff / 5.0f;
	}

	// Apply Rotation
	currentAngle_ += steeringInput * turnSpeed_ * dt;

	// Apply Forward Movement
	// Recalculate direction based on new angle
	Vector2D newDir = Vector2D::CircularMotion({ 0,0 }, 1.0f, currentAngle_ * DEG2RAD);
	Vector2D velocity = newDir.ScaleVector(speed_ * dt);
	position_ = position_.SetVectorOffset(velocity);

	// Tail Logic
	//Grow Length Smoothly
	tailLengthPixels_ += growthRatePixelsPerSecond_ * dt;

	// Parent class helper to update tail segments
	UpdateTailLogic({ position_.x, position_.y });
	
}

// Draw owner (tail + head)
void Enemy::OwnerDraw()
{
	if (!isAlive_) return;

	// Draw Tail
	if (tailSegments_.size() > 1)
	{
		for (size_t i = 0; i < tailSegments_.size() - 1; ++i)
		{
			// DrawLineV draws a standard thin line between two points
			DrawLineV(tailSegments_[i], tailSegments_[i + 1], Fade(color_, 0.5f));
		}
	}

	// Draw Head
	// Convert logic position (Vector2D) to render position (Vector2)
	Vector2 drawPos = { position_.x - size_.x / 2.0f, position_.y - size_.y / 2.0f };
	DrawRectangleV(drawPos, size_, color_);
}

//Various getters and status checkers below

// Kill the owner (set dead)
void Enemy::OwnerKill()
{
	isAlive_ = false;
	ResetTail({ position_.x, position_.y });
}

// Check if owner is alive
bool Enemy::IsAlive() const
{
	return isAlive_;
}

// Get head position (top-left)
Vector2 Enemy::GetPosition() const
{
	return { position_.x, position_.y };
}

// Get head size
Vector2 Enemy::GetSize() const
{
	return size_;
}
