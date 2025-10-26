#include "mPlayer.h"
#include "raymath.h"

//Constructor implement
mPlayer::mPlayer(Vector2 pStartPos, Vector2 pSize, float pSpeed, float pRotationSpeed, Color pColor, int pKeyLeft, int pKeyRight)
	: 
	m_position(pStartPos),
	m_size(pSize),
	m_speed(pSpeed),
	m_rotationSpeed(pRotationSpeed),
	m_directionAngle(90.0f), 
	m_color(pColor),
	m_isAlive(true),
	m_tailLengthPixels(200.0f),     // default total tail length in pixels
	m_trailSampleDistance(3.0f),    // default min spacing between stored points
	m_maxTrailPoints(0),
	m_tailGrowthEnabled(true),
	m_growthRatePixelsPerSecond(40.0f),      // pixels per second when continuous
	m_keyLeft(pKeyLeft),
	m_keyRight(pKeyRight)
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
}

void mPlayer::SetControlKeys(int leftKey, int rightKey)
{
	m_keyLeft = leftKey;
	m_keyRight = rightKey;
}

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

void mPlayer::SetTailGrowthEnabled(bool enabled)
{
	m_tailGrowthEnabled = enabled;
}

void mPlayer::SetTailGrowthRate(float pixelsPerSecond)
{
	if (pixelsPerSecond >= 0.0f)
		m_growthRatePixelsPerSecond = pixelsPerSecond;
}

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

void mPlayer::mPUpdate(float deltaTime)
{
	if (!m_isAlive) return;

	// 1. Calculate the normalized direction vector from the angle
	float angleRad = m_directionAngle * DEG2RAD;
	Vector2 direction = { cosf(angleRad), sinf(angleRad) };

	// 2. Calculate velocity (direction * speed)
	Vector2 velocity = Vector2Scale(direction, m_speed);

	// 3. Update position (position + velocity * deltaTime)
	m_position = Vector2Add(m_position, Vector2Scale(velocity, deltaTime));

	// 4. SAMPLE the new position into the trail only when the player moved enough
	Vector2 last = m_trail.empty() ? m_position : m_trail.back();
	float dist = Vector2Distance(last, m_position);

	if (dist >= m_trailSampleDistance)
	{
		m_trail.push_back(m_position);
	}
	else
	{
		// keep last point updated so head doesn't lag visually
		if (!m_trail.empty())
			m_trail.back() = m_position;
		else
			m_trail.push_back(m_position);
	}

	// 5. Continuous growth: add pixels to allowed tail length each frame (smooth)
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

//Draws the snake
void mPlayer::mPDraw()
{
	// 1. Draw the trail
	if (m_trail.size() > 1)
	{
		for (size_t i = 0; i < m_trail.size() - 1; ++i)
		{
			DrawLineV(m_trail[i], m_trail[i + 1], m_color);
		}
	}

	// 2. Draw the player's head
	if (!m_isAlive) return;
	Rectangle rec = { m_position.x, m_position.y, m_size.x, m_size.y };
	Vector2 origin = { m_size.x / 2.0f, m_size.y / 2.0f };
	DrawRectanglePro(rec, origin, m_directionAngle, m_color);
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
}

void mPlayer::CutTrail(size_t cutIndex)
{
	if (cutIndex >= m_trail.size()) return;
	// remove front segments up to cutIndex (keeps head)
	for (size_t i = 0; i < cutIndex && !m_trail.empty(); ++i)
		m_trail.pop_front();
}