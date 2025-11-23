#include "Enemy.h"
#include <cmath>

Enemy::Enemy(Vector2 startPos, float speed, Color color)
	: position_({ startPos.x, startPos.y })
	, speed_(speed)
	, color_(color)
	, size_({ 16.0f, 16.0f })
	, isAlive_(true)
	, currentTarget_({ 0, 0 })
	, timeSinceLastTailDrop_(0.0f)
	, tailDropInterval_(0.1f)
{
}

void Enemy::SetTarget(Vector2 targetPos)
{
	// Convert Raylib target to your math class
	currentTarget_ = { targetPos.x, targetPos.y };
}

void Enemy::OwnerUpdate(float dt)
{
	if (!isAlive_) return;

	// Get vector to target
	Vector2D direction = position_.VectorTowardsTarget(currentTarget_);

	// Normalize it
	Vector2D unitDir = direction.NormalizeVector();

	// Scale by speed
	Vector2D velocity = unitDir.ScaleVector(speed_ * dt);

	// Apply movement
	position_ = position_.SetVectorOffset(velocity);

	// --- TAIL LOGIC ---
	timeSinceLastTailDrop_ += dt;
	if (timeSinceLastTailDrop_ >= tailDropInterval_)
	{
		// Convert your Vector2D back to Raylib Vector2 for the interface deque
		tailSegments_.push_front({ position_.x, position_.y });

		if (tailSegments_.size() > 100) // Cap tail length
		{
			tailSegments_.pop_back();
		}
		timeSinceLastTailDrop_ = 0.0f;
	}
}

void Enemy::OwnerDraw()
{
	if (!isAlive_) return;

	// Draw Tail
	for (const auto& segment : tailSegments_)
	{
		DrawRectangleV(segment, size_, Fade(color_, 0.5f));
	}

	// Draw Head
	// Convert logic position (Vector2D) to render position (Vector2)
	Vector2 drawPos = { position_.x, position_.y };
	DrawRectangleV(drawPos, size_, color_);
}

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
	// Interface needs Raylib Vector2
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
	return (float)tailSegments_.size() * (speed_ * tailDropInterval_);
}