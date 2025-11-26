#pragma once
#include "Vector2D.h"
#include <raylib.h>

enum class PowerUpType
{
	SpeedBoost, // Green
	SlowDown,   // "Fake" Green (looks similar, behaves opposite)
	PhaseShift, // Blue (Ghost mode)
	TailCutter  // Gold (Cut tail on impact (currently just destroys)
};

class PowerUp
{
public:
	PowerUp(Vector2D startPos, PowerUpType type);

	//  Keeping track of active state
	void Update(float dt);
	void Draw();

	// Get bounding rectangle for collision detection
	Rectangle GetRect() const;

	// Accessors
	bool IsActive() const;
	void Deactivate();
	PowerUpType GetType() const;

private:
	// Data Members
	PowerUpType type_;
	Vector2D position_;
	bool active_;

	Color color_;
	float size_;
};