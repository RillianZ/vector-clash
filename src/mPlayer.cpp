#include "mPlayer.h"
#include "raymath.h"

//Constructor implement : can accept custom control
mPlayer::mPlayer(Vector2 pStartPos, Vector2 pSize, float pSpeed, float pRotationSpeed, Color pColor, int pKeyLeft, int pKeyRight)
	:
	position_(pStartPos), // starting position
	size_(pSize), // size of head
	speed_(pSpeed), // pixels per second
	baseSpeed_(pSpeed), // store base speed
	rotationSpeed_(pRotationSpeed), // degrees per second rotate
	directionAngle_(90.0f), // facing down initially
	color_(pColor), // default color
	isAlive_(true), // player starts alive
	tailGrowthEnabled_(true),   // tail grows continuously by default
	growthRatePixelsPerSecond_(40.0f),      // pixels per second when continuous
	keyLeft_(pKeyLeft), //Standard key for left movement
	keyRight_(pKeyRight), //Standard key for right movement
	speedTimer_(0.0f),       //timers for power-ups (speed)
	ghostTimer_(0.0f),       //timers for power-ups (ghost)
	hasTailCutter_(false)  // starts without cutter
{
	ResetTail(position_);
}

// Reset player to an initial state (used for restart)
void mPlayer::Reset(Vector2 pStartPos, int pKeyLeft, int pKeyRight, float pStartAngle)
{
	// reset basic state
	position_ = pStartPos;
	directionAngle_ = pStartAngle;
	isAlive_ = true;

	// reset controls
	keyLeft_ = pKeyLeft;
	keyRight_ = pKeyRight;

	// reset powerups and base speed
	speed_ = baseSpeed_;
	speedTimer_ = 0.0f;
	ghostTimer_ = 0.0f;
	hasTailCutter_ = false;

	// Reset tail to default length and clear history
	tailLengthPixels_ = 200.0f;
	ResetTail(position_);
}

//sets the control keys for the player
void mPlayer::SetControlKeys(int leftKey, int rightKey)
{
	keyLeft_ = leftKey;
	keyRight_ = rightKey;
}

// Enables or disables continuous tail growth
void mPlayer::SetTailGrowthEnabled(bool enabled)
{
	tailGrowthEnabled_ = enabled;
}

// Sets the continuous tail growth rate in pixels per second
void mPlayer::SetTailGrowthRate(float pixelsPerSecond)
{
	if (pixelsPerSecond >= 0.0f)
		growthRatePixelsPerSecond_ = pixelsPerSecond;
}

// Apply PowerUp Effects
void mPlayer::ApplyPowerUp(PowerUpType type)
{
	tailSegments_.push_back(position_);

	switch (type)
	{
	case PowerUpType::SpeedBoost:
		speed_ = baseSpeed_ * 1.5f; // 50% faster
		speedTimer_ = 3.0f;          // Lasts 3 seconds
		break;

	case PowerUpType::SlowDown:
		speed_ = baseSpeed_ * 0.5f; // 50% slower
		speedTimer_ = 3.0f;
		break;

	case PowerUpType::PhaseShift:
		ghostTimer_ = 5.0f;          // 5 seconds of invulnerability
		break;

	case PowerUpType::TailCutter:
		hasTailCutter_ = true;       // Lasts until used
		break;
	}
}

// Status Getters for phasing
bool mPlayer::IsPhasing() const { return ghostTimer_ > 0.0f; }
// Status Getters for cutter availability
bool mPlayer::HasTailCutter() const { return hasTailCutter_; }
// Status Getters for cutter consumption
void mPlayer::ConsumeTailCutter() { hasTailCutter_ = false; }

// Clears the tail segments from player
void mPlayer::ClearTail()
{
	ResetTail(position_);
}

// Use per-instance keys if set by constructor, otherwise defaults KEY_LEFT/KEY_RIGHT
//Handles player input for rotation
void mPlayer::HandleInput(float deltaTime)
{
	if (IsKeyDown(keyLeft_))
	{
		directionAngle_ -= rotationSpeed_ * deltaTime;
	}

	if (IsKeyDown(keyRight_))
	{
		directionAngle_ += rotationSpeed_ * deltaTime;
	}
}

//Updates the player position and trail
void mPlayer::Update(float deltaTime)
{
	if (!isAlive_) return;

	if (speedTimer_ > 0.0f)
	{
		speedTimer_ -= deltaTime;
		if (speedTimer_ <= 0.0f) speed_ = baseSpeed_;
	}
	if (ghostTimer_ > 0.0f)
	{
		ghostTimer_ -= deltaTime;
	}

	//Calculate the normalized direction vector from the angle
	Vector2D direction = Vector2D::CircularMotion({ 0,0 }, 1.0f, directionAngle_ * DEG2RAD);

	// Calculate velocity (direction * speed)
	Vector2D velocity = direction.ScaleVector(speed_ * deltaTime);

	Vector2D currentPos = { position_.x, position_.y };
	Vector2D newPos = currentPos.SetVectorOffset(velocity);

	// Update position (position + velocity * deltaTime)
	position_ = { newPos.x, newPos.y };

	// Continuous growth: add pixels to allowed tail length each frame (smooth)
	if (tailGrowthEnabled_)
	{
		tailLengthPixels_ += growthRatePixelsPerSecond_ * deltaTime;
	}

	// Adjust trail sample distance based on speed status
	//to ensure smooth trail at different speeds
	if (speed_ < baseSpeed_)
	{
		trailSampleDistance_ = 1.5f;
	}
	else
	{
		trailSampleDistance_ = 3.0f; // Default
	}

	// Delegate the heavy lifting to the parent class!
	UpdateTailLogic(position_);

}

// Simple rectangle of the player's head (for collision checks)
Rectangle mPlayer::GetRect() const
{
	// Convert center position to top-left for collision logic
	return {
		position_.x - size_.x / 2.0f,
		position_.y - size_.y / 2.0f,
		size_.x,
		size_.y
	};
}

//Draws the snake
void mPlayer::Draw()
{
	// Draw the trail
	if (tailSegments_.size() > 1)
	{
		Color trailColor = color_;
		if (ghostTimer_ > 0.0f) trailColor = Fade(color_, 0.2f);
		for (size_t i = 0; i < tailSegments_.size() - 1; ++i)
		{
			DrawLineV(tailSegments_[i], tailSegments_[i + 1], trailColor);
		}
	}

	// checks if alive before drawing head
	if (!isAlive_) return;

	// initialize head color
	Color headColor = color_;

	// Speed Status Logic
	if (speedTimer_ > 0.0f)
	{
		// Create a "strobe" effect by checking time
		bool flash = (int)(GetTime() * 15.0f) % 2 == 0;

		if (speed_ > baseSpeed_)
		{
			// SPEED BOOST: Flash Green
			if (flash) headColor = RAYWHITE;
		}
		else if (speed_ < baseSpeed_)
		{
			// SLOW DOWN: Flash Red
			if (flash) headColor = DARKGRAY;
		}
	}

	// Fade to sky blue when ghosting
	if (ghostTimer_ > 0.0f)
	{
		headColor = Fade(SKYBLUE, 0.3f);
	}

	// Define rectangle for head
	Rectangle rec = { position_.x, position_.y, size_.x, size_.y };
	Vector2 origin = { size_.x / 2.0f, size_.y / 2.0f };
	// Draw head rectangle with rotation
	DrawRectanglePro(rec, origin, directionAngle_, headColor);

	// Draw tail cutter indicator if active
	if (hasTailCutter_)
	{
		DrawRectanglePro(rec, origin, directionAngle_, Fade(GOLD, 0.5f));
	}

	float activeTimer = 0.0f;
	Color textColor = WHITE;

	if (speedTimer_ > 0.0f) { activeTimer = speedTimer_; textColor = YELLOW; }
	if (ghostTimer_ > 0.0f) { activeTimer = ghostTimer_; textColor = SKYBLUE; }

	if (activeTimer > 0.0f)
	{
		// Convert to string with 1 decimal place
		std::string timeText = std::to_string(activeTimer).substr(0, 3);

		// Position text slightly above the player
		DrawText(timeText.c_str(), (int)position_.x - 10, (int)position_.y - 25, 10, textColor);
	}
}

// Lifecycle hooks for game manager
void mPlayer::OwnerUpdate(float deltaTime)
{
	HandleInput(deltaTime);
	Update(deltaTime);
}

// Draws the player and its trail
void mPlayer::OwnerDraw()
{
	Draw();
}

// Calls mPKill to kill the player
void mPlayer::OwnerKill()
{
	Kill();
}

// Simple accessors and utilities used by main collision logic
Vector2 mPlayer::GetPosition() const
{
	return position_;
}

// Returns the size of the player's head
Vector2 mPlayer::GetSize() const
{
	return size_;
}

// Returns whether the player is alive
bool mPlayer::IsAlive() const
{
	return isAlive_;
}

// Kills the player
void mPlayer::Kill()
{
	isAlive_ = false;
	ResetTail(position_);
}

// Cuts the trail by removing the specified number of oldest segments
void mPlayer::CutTrail(size_t cutIndex)
{
	if (cutIndex >= tailSegments_.size()) return;
	for (size_t i = 0; i < cutIndex && !tailSegments_.empty(); ++i)
		tailSegments_.pop_front();
}