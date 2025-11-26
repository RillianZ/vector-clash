#include "Vector2D.h"

//This whole class is used for math and calculations
//I reworked to use return by value instead of reference parameters

//calculates the magnitude of the vector
float Vector2D::CalculateMagnitude() const
{
	return  sqrtf(x * x + y * y);;
}

//adds two vectors together and returns the result
Vector2D Vector2D::SetVectorOffset(Vector2D inVectorToAdd) const
{
	return { x + inVectorToAdd.x, y + inVectorToAdd.y };
}

//scales the vector by a scalar value
Vector2D Vector2D::ScaleVector(float inScalar) const
{
	return { x * inScalar, y * inScalar };
}

//calculates the vector pointing towards a target vector
Vector2D Vector2D::VectorTowardsTarget(Vector2D inTargetVector) const
{
	return { inTargetVector.x - x, inTargetVector.y - y };
}

//calculates the distance to a target vector
float Vector2D::DistanceToTarget(Vector2D inTargetedVector) const
{
	return VectorTowardsTarget(inTargetedVector).CalculateMagnitude();
}

//normalizes the vector to have a magnitude of 1
// if the magnitude is 0, returns a zero vector to avoid division by zero
Vector2D Vector2D::NormalizeVector() const
{
	float mag = CalculateMagnitude();
	if (mag == 0) return { 0, 0 };
	return { x / mag, y / mag };
}

// Rotates a vector using the 2D Rotation Matrix
Vector2D Vector2D::Rotate(float angle) const
{
	float cosRes = cosf(angle);
	float sinRes = sinf(angle);

	// Formula: x' = x cos - y sin, y' = x sin + y cos
	float newX = x * cosRes - y * sinRes;
	float newY = x * sinRes + y * cosRes;

	return { newX, newY };
}

// Returns the angle of the vector in radians (atan2)
// atan2 returns angle in range -PI to PI
// angle 0 is along positive x-axis, PI/2 is along positive y-axis
float Vector2D::GetAngle() const
{
	return atan2f(y, x);
}

// Generates a point moving in a cosine wave pattern
// along the x-axis around a center point
// inAmplitude controls the wave height
// inAngle controls the phase of the wave
Vector2D Vector2D::CosineMovement(Vector2D inCenter, float inAmplitude, float inAngle)
{
	return { inCenter.x + inAmplitude * cosf(inAngle), inCenter.y };
}

// Generates a point moving in a circular motion
Vector2D Vector2D::CircularMotion(Vector2D inCenter, float inRadius, float inAngle)
{
	return { inCenter.x + inRadius * cosf(inAngle), inCenter.y + inRadius * sinf(inAngle) };
}

// Normalizes an angle difference to the range -180 to 180 degrees
float Vector2D::NormalizeAngleDiff(float diff)
{
	while (diff > 180.0f) diff -= 360.0f;
	while (diff < -180.0f) diff += 360.0f;
	return diff;
}