#include "Enemy.h"
#include <cmath>
#include "mPlayer.h"
#include "raymath.h"

float NormalizeAngleDiff(float diff)
{
	while (diff > 180.0f) diff -= 360.0f;
	while (diff < -180.0f) diff += 360.0f;
	return diff;
}

Enemy::Enemy(Vector2 startPos, Vector2 size, float speed, float turnSpeed, Color color, int screenW, int screenH)
	: position_({ startPos.x, startPos.y })
	, turnSpeed_(turnSpeed)
	, speed_(speed)
	, color_(color)
	, size_(size)
	, isAlive_(true)
	, currentTarget_({ 0, 0 })
	, currentAngle_(0.0f)
	, playerRef_(nullptr)
	, screenWidth_(screenW)
	, screenHeight_(screenH)

{
	tailSegments_.push_back({ position_.x, position_.y });
}

void Enemy::SetPlayerReference(mPlayer* playerRef)
{
	playerRef_ = playerRef;
}

void Enemy::SetTarget(Vector2 targetPos)
{
	currentTarget_ = { targetPos.x, targetPos.y };
}

void Enemy::Reset(Vector2 startPos)
{
	position_ = { startPos.x, startPos.y };
	tailSegments_.clear();
	currentAngle_ = 90.0f; // Reset angle (90 = Down)
	tailSegments_.push_back({ position_.x, position_.y });
	isAlive_ = true;
}

// The "Enemy Cone Check" function
bool Enemy::CheckCollisionAhead(Vector2D start, Vector2D dir, float lookAheadDist)
{
	// Calculate the tip of the feelers using Vector2D math
	Vector2D tip = start.SetVectorOffset(dir.ScaleVector(lookAheadDist));

	//  Wall Checks
	const int buffer = 20;
	if (tip.x < buffer || tip.x > screenWidth_ - buffer || tip.y < buffer || tip.y > screenHeight_ - buffer) return true;

	//  Self Tail Chec
	size_t skipRecent = 20;
	if (tailSegments_.size() > skipRecent)
	{
		for (size_t i = 0; i < tailSegments_.size() - skipRecent; i += 5)
		{
			Vector2D seg = { tailSegments_[i].x, tailSegments_[i].y };
			// If we are about to hit a tail segment
			if (tip.DistanceToTarget(seg) < size_.x * 1.2f) return true;
		}
	}

	// Player Tail Check
	// uses playerRef_ pointer to access player's tail segments
	if (playerRef_ && playerRef_->IsAlive())
	{
		const auto& pTail = playerRef_->GetTailSegments();
		for (size_t i = 0; i < pTail.size(); i += 5)
		{
			Vector2D seg = { pTail[i].x, pTail[i].y };
			if (tip.DistanceToTarget(seg) < size_.x) return true;
		}
	}

	return false;
}

void Enemy::OwnerUpdate(float dt)
{
	if (!isAlive_) return;

	// STEERING & MOVEMENT

	//  Calculate Forward Direction
	// CLEANER: Calling static method Vector2D::CircularMotion
	Vector2D currentDir = Vector2D::CircularMotion({ 0,0 }, 1.0f, currentAngle_ * DEG2RAD);
	float lookAhead = speed_ * 0.5f;

	// Calculate Whiskers using ROTATION
	float whiskerAngleRad = 0.6f;
	Vector2D leftDir = currentDir.Rotate(-whiskerAngleRad);
	Vector2D rightDir = currentDir.Rotate(whiskerAngleRad);

	//Collision Checks
	bool hitFront = CheckCollisionAhead(position_, currentDir, lookAhead);
	bool hitLeft = CheckCollisionAhead(position_, leftDir, lookAhead * 0.8f);
	bool hitRight = CheckCollisionAhead(position_, rightDir, lookAhead * 0.8f);

	//  Steering Input
	float steeringInput = 0.0f;

	if (hitFront || hitLeft || hitRight)
	{
		if (hitFront)
		{
			if (hitLeft && !hitRight) steeringInput = 1.0f;
			else if (hitRight && !hitLeft) steeringInput = -1.0f;
			else steeringInput = 1.0f;
		}
		else if (hitLeft) steeringInput = 0.8f;
		else if (hitRight) steeringInput = -0.8f;
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

	// TAIL LOGIC
	Vector2 currentPosRay = Vector2{ position_.x, position_.y };

	if (tailSegments_.empty())
	{
		tailSegments_.push_back(currentPosRay);
		return;
	}

	Vector2 last = tailSegments_.back();
	Vector2D lastVec = { last.x, last.y };

	if (position_.DistanceToTarget(lastVec) >= 3.0f)
	{
		tailSegments_.push_back(currentPosRay);

		if (tailSegments_.size() > 30000)
		{
			tailSegments_.pop_front();
		}
	}
}

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

//Various getters and status checkers
//getters send back position, size, tail segments, tail length in pixels
//status checker returns if enemy is alive and kill function sets isAlive to false and clears tail segments
void Enemy::OwnerKill()
{
	isAlive_ = false;
	tailSegments_.clear();
}

bool Enemy::IsAlive() const
{
	return isAlive_;
}

Vector2 Enemy::GetPosition() const
{
	return { position_.x, position_.y };
}

Vector2 Enemy::GetSize() const
{
	return size_;
}

const std::deque<Vector2>& Enemy::GetTailSegments() const
{
	return tailSegments_;
}

float Enemy::GetTailPixelLength() const
{
	const float sampleDistance = 3.0f;
	return (float)tailSegments_.size() * sampleDistance;
}