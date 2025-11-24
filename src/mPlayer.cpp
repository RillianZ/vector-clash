#include "mPlayer.h"
#include "raymath.h"

//Constructor implement
mPlayer::mPlayer(Vector2 pStartPos, Vector2 pSize, float pSpeed, float pRotationSpeed, Color pColor, int pKeyLeft, int pKeyRight)
	:
	m_position(pStartPos), // starting position
	m_size(pSize), // size of head
	m_speed(pSpeed), // pixels per second
	m_baseSpeed(pSpeed), // store base speed
	m_rotationSpeed(pRotationSpeed), // degrees per second rotate
	m_directionAngle(90.0f), // facing down initially
	m_color(pColor), // default color
	m_isAlive(true), // player starts alive
	m_tailLengthPixels(200.0f),     // default total tail length in pixels
	m_trailSampleDistance(3.0f),    // default min spacing between stored points
	m_maxTrailPoints(0), // this will be computed below in ctor body
	m_tailGrowthEnabled(true),   // tail grows continuously by default
	m_growthRatePixelsPerSecond(40.0f),      // pixels per second when continuous
	m_keyLeft(pKeyLeft), //Standard key for left movement
	m_keyRight(pKeyRight), //Standard key for right movement
	m_speedTimer(0.0f),       //timers for power-ups
	m_ghostTimer(0.0f),       
	m_hasTailCutter(false)  
{
	m_maxTrailPoints = static_cast<size_t>(m_tailLengthPixels / m_trailSampleDistance);
	if (m_maxTrailPoints < 2) m_maxTrailPoints = 2; // Ensure at least 2 points
	m_trail.clear();
	m_trail.push_back(m_position);
}

// Reset player to an initial state (used for restart)
void mPlayer::Reset(Vector2 pStartPos, int pKeyLeft, int pKeyRight, float pStartAngle)
{
	// reset basic state
	m_position = pStartPos;
	m_directionAngle = pStartAngle;
	m_isAlive = true;

	// reset controls
	m_keyLeft = pKeyLeft;
	m_keyRight = pKeyRight;

	// reset tail params to defaults (keep same defaults as ctor)
	m_tailLengthPixels = 200.0f;
	m_trailSampleDistance = 3.0f;
	m_maxTrailPoints = static_cast<size_t>(m_tailLengthPixels / m_trailSampleDistance);
	if (m_maxTrailPoints < 2) m_maxTrailPoints = 2;

	// clear and seed trail
	m_trail.clear();
	m_trail.push_back(m_position);

	// reset powerups and base speed
	m_speed = m_baseSpeed;
	m_speedTimer = 0.0f;
	m_ghostTimer = 0.0f;
	m_hasTailCutter = false;
}

//sets the control keys for the player
void mPlayer::SetControlKeys(int leftKey, int rightKey)
{
	m_keyLeft = leftKey;
	m_keyRight = rightKey;
}

// Sets the total tail length in pixels
void mPlayer::SetTailLength(float pixels)
{
	if (pixels <= 0.0f) return;
	m_tailLengthPixels = pixels;
	m_maxTrailPoints = static_cast<size_t>(m_tailLengthPixels / m_trailSampleDistance);
	if (m_maxTrailPoints < 2) m_maxTrailPoints = 2;

	// trim existing trail if necessary
	while (m_trail.size() > m_maxTrailPoints)
		m_trail.pop_front();
}

// Enables or disables continuous tail growth
void mPlayer::SetTailGrowthEnabled(bool enabled)
{
	m_tailGrowthEnabled = enabled;
}

// Sets the continuous tail growth rate in pixels per second
void mPlayer::SetTailGrowthRate(float pixelsPerSecond)
{
	if (pixelsPerSecond >= 0.0f)
		m_growthRatePixelsPerSecond = pixelsPerSecond;
}

// Apply PowerUp Effects
void mPlayer::ApplyPowerUp(PowerUpType type)
{
	m_trail.push_back(m_position);

	switch (type)
	{
	case PowerUpType::SpeedBoost:
		m_speed = m_baseSpeed * 1.5f; // 50% faster
		m_speedTimer = 3.0f;          // Lasts 3 seconds
		break;

	case PowerUpType::SlowDown:
		m_speed = m_baseSpeed * 0.5f; // 50% slower
		m_speedTimer = 3.0f;
		break;

	case PowerUpType::PhaseShift:
		m_ghostTimer = 5.0f;          // 5 seconds of invulnerability
		break;

	case PowerUpType::TailCutter:
		m_hasTailCutter = true;       // Lasts until used
		break;
	}
}

// Status Getters
bool mPlayer::IsPhasing() const { return m_ghostTimer > 0.0f; }
bool mPlayer::HasTailCutter() const { return m_hasTailCutter; }
void mPlayer::ConsumeTailCutter() { m_hasTailCutter = false; }

void mPlayer::ClearTail()
{
	m_trail.clear();
	m_trail.push_back(m_position);
}

//Handles player input for rotation
void mPlayer::mPHandleInput(float deltaTime)
{
	// Use per-instance keys if set; otherwise defaults KEY_LEFT/KEY_RIGHT
	if (IsKeyDown(m_keyLeft))
	{
		m_directionAngle -= m_rotationSpeed * deltaTime;
	}

	if (IsKeyDown(m_keyRight))
	{
		m_directionAngle += m_rotationSpeed * deltaTime;
	}
}

//Updates the player position and trail
void mPlayer::mPUpdate(float deltaTime)
{
	if (!m_isAlive) return;

	// speed timer countdown
	if (m_speedTimer > 0.0f)
	{
		m_speedTimer -= deltaTime;
		if (m_speedTimer <= 0.0f) m_speed = m_baseSpeed; 
	}
	// ghost timer countdown
	if (m_ghostTimer > 0.0f)
	{
		m_ghostTimer -= deltaTime;
	}

	float currentSampleDist = (m_speed < m_baseSpeed) ? 1.5f : m_trailSampleDistance;

	//Calculate the normalized direction vector from the angle
	Vector2D direction = Vector2D::CircularMotion({ 0,0 }, 1.0f, m_directionAngle * DEG2RAD);

	// Calculate velocity (direction * speed)
	Vector2D velocity = direction.ScaleVector(m_speed * deltaTime);

	Vector2D currentPos = { m_position.x, m_position.y };
	Vector2D newPos = currentPos.SetVectorOffset(velocity);

	// Update position (position + velocity * deltaTime)
	m_position = { newPos.x, newPos.y };

	//sample the new position into the trail only when the player moved enough
	Vector2 last = m_trail.empty() ? m_position : m_trail.back();
	Vector2D lastVec = { last.x, last.y };
	Vector2D currVec = { m_position.x, m_position.y };

	if (lastVec.DistanceToTarget(currVec) >= currentSampleDist)
	{
		m_trail.push_back(m_position);
	}
	else if  (!m_trail.empty()) m_trail.back() = m_position;

	// Continuous growth: add pixels to allowed tail length each frame (smooth)
	if (m_tailGrowthEnabled)
	{
		m_tailLengthPixels += m_growthRatePixelsPerSecond * deltaTime;
		// recompute max points from pixel length and sample distance
		m_maxTrailPoints = static_cast<size_t>(m_tailLengthPixels / m_trailSampleDistance);
		if (m_maxTrailPoints < 2) m_maxTrailPoints = 2;
	}

	// 6. Trim oldest parts of the trail so the tail "follows"
	while (m_trail.size() > m_maxTrailPoints)
	{
		m_trail.pop_front();
	}
}

// Simple rectangle of the player's head (for collision checks)
Rectangle mPlayer::GetRect() const
{
	// Convert center position to top-left for collision logic
	return {
		m_position.x - m_size.x / 2.0f,
		m_position.y - m_size.y / 2.0f,
		m_size.x,
		m_size.y
	};
}

//Draws the snake
void mPlayer::mPDraw()
{
	// Draw the trail
	if (m_trail.size() > 1)
	{
		Color trailColor = m_color;
		if (m_ghostTimer > 0.0f) trailColor = Fade(m_color, 0.2f);
		for (size_t i = 0; i < m_trail.size() - 1; ++i)
		{
			DrawLineV(m_trail[i], m_trail[i + 1], trailColor);
		}		
	}

	// checks if alive before drawing head
	if (!m_isAlive) return;

	// initialize head color
	Color headColor = m_color;

	// Speed Status Logic
	if (m_speedTimer > 0.0f)
	{
		// Create a "strobe" effect by checking time
		// Flashes every few frames
		bool flash = (int)(GetTime() * 15.0f) % 2 == 0;

		if (m_speed > m_baseSpeed)
		{
			// SPEED BOOST: Flash Green
			if (flash) headColor = RAYWHITE;
		}
		else if (m_speed < m_baseSpeed)
		{
			// SLOW DOWN: Flash Red
			if (flash) headColor = DARKGRAY;
		}
	}

	// Fade to sky blue when ghosting
	if (m_ghostTimer > 0.0f)
	{
		headColor = Fade(SKYBLUE, 0.3f);
	}

	// Define rectangle for head
	Rectangle rec = { m_position.x, m_position.y, m_size.x, m_size.y };
	Vector2 origin = { m_size.x / 2.0f, m_size.y / 2.0f };
	// Draw head rectangle with rotation
	DrawRectanglePro(rec, origin, m_directionAngle, headColor);

	// Draw tail cutter indicator if active
	if (m_hasTailCutter)
	{
		DrawRectanglePro(rec, origin, m_directionAngle, Fade(GOLD, 0.5f));
	}

	float activeTimer = 0.0f;
	Color textColor = WHITE;

	if (m_speedTimer > 0.0f) { activeTimer = m_speedTimer; textColor = YELLOW; }
	if (m_ghostTimer > 0.0f) { activeTimer = m_ghostTimer; textColor = SKYBLUE; }

	if (activeTimer > 0.0f)
	{
		// Convert to string with 1 decimal place
		std::string timeText = std::to_string(activeTimer).substr(0, 3);

		// Position text slightly above the player
		DrawText(timeText.c_str(), (int)m_position.x - 10, (int)m_position.y - 25, 10, textColor);
	}
}

// Lifecycle hooks (ITailOwner)
void mPlayer::OwnerUpdate(float deltaTime)
{
	// combine input + update so managers can call one method
	mPHandleInput(deltaTime);
	mPUpdate(deltaTime);
}

void mPlayer::OwnerDraw()
{
	mPDraw();
}

void mPlayer::OwnerKill()
{
	mPKill();
}

float mPlayer::GetTailPixelLength() const
{
	return m_tailLengthPixels;
}

// Simple accessors and utilities used by main collision logic
Vector2 mPlayer::GetPosition() const
{
	return m_position;
}

Vector2 mPlayer::GetSize() const
{
	return m_size;
}

const std::deque<Vector2>& mPlayer::GetTailSegments() const
{
	return m_trail;
}

bool mPlayer::IsAlive() const
{
	return m_isAlive;
}

void mPlayer::mPKill()
{
	m_isAlive = false;
	ClearTail();
}

void mPlayer::CutTrail(size_t cutIndex)
{
	if (cutIndex >= m_trail.size()) return;
	// remove front segments up to cutIndex (keeps head)
	for (size_t i = 0; i < cutIndex && !m_trail.empty(); ++i)
		m_trail.pop_front();
}