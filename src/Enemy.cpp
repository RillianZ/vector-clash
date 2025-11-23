#include "Enemy.h"
#include <cmath>

Enemy::Enemy(Vector2 startPos, Vector2 size, float speed, Color color)
	: position_({ startPos.x, startPos.y })
	, speed_(speed)
	, color_(color)
	, size_(size)
	, isAlive_(true)
	, currentTarget_({ 0, 0 })
{
}

void Enemy::SetTarget(Vector2 targetPos)
{
	// Convert Raylib target to your math class
	currentTarget_ = { targetPos.x, targetPos.y };
}

void Enemy::Reset(Vector2 startPos)
{
	// Reset Logic using your Vector2D class
	position_ = { startPos.x, startPos.y };
	tailSegments_.clear();
	isAlive_ = true;
}

void Enemy::OwnerUpdate(float dt)
{
	if (!isAlive_) return;

	// --- MOVEMENT (Your Vector2D Logic) ---
	Vector2D direction = position_.VectorTowardsTarget(currentTarget_);
	Vector2D unitDir = direction.NormalizeVector();
	Vector2D velocity = unitDir.ScaleVector(speed_ * dt);
	position_ = position_.SetVectorOffset(velocity);

	// --- TAIL SAMPLING ---
	Vector2 currentPosRay = Vector2{ position_.x, position_.y };

	// --- TAIL LOGIC (Using Vector2D Math) ---
	Vector2 last = tailSegments_.empty() ? Vector2{position_.x, position_.y} : tailSegments_.back();

	// Convert 'last' (Raylib Vector2) to 'lastVec' (Your Vector2D)
	Vector2D lastVec = { last.x, last.y };


	// Use the class to check distance. 
	if (position_.DistanceToTarget(lastVec) >= 3.0f)
	{
		tailSegments_.push_back(currentPosRay);

		// Limit based on rough point count (3000 * 3px = ~9000px tail)
		if (tailSegments_.size() > 3000)
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
	const float sampleDistance = 3.0f;
	return (float)tailSegments_.size() * sampleDistance;
}