#include "PowerUp.h"

// Constructor
PowerUp::PowerUp(Vector2D startPos, PowerUpType type)
	: position_(startPos)
	, type_(type)
	, active_(true)
	, color_(WHITE)
	, size_(16.0f)
{
	// Assign color based on type
	switch (type_)
	{
	case PowerUpType::SpeedBoost:
		color_ = GREEN;
		break;
	case PowerUpType::SlowDown:
		color_ = LIME;
		break;
	case PowerUpType::PhaseShift:
		color_ = SKYBLUE;
		break;
	case PowerUpType::TailCutter:
		color_ = GOLD;
		break;
	}
}

// Update method (currently static implemented for later moving powerups)
void PowerUp::Update(float dt)
{
	// Static: No movement logic needed until i add moving powerups later
}

// Draw method
void PowerUp::Draw()
{
	if (!active_) return;

	// Simple rectangle to match player aesthetic
	DrawRectangle((int)position_.x, (int)position_.y, (int)size_, (int)size_, color_);

	// A thin border helps distinguish it from background
	DrawRectangleLines((int)position_.x, (int)position_.y, (int)size_, (int)size_, RAYWHITE);
}

// Get bounding rectangle for collision detection
Rectangle PowerUp::GetRect() const
{
	return { position_.x, position_.y, size_, size_ };
}

// Accessors
bool PowerUp::IsActive() const
{
	return active_;
}

// Deactivate the power-up
void PowerUp::Deactivate()
{
	active_ = false;
}
// Get the type of the power-up
PowerUpType PowerUp::GetType() const
{
	return type_;
}