#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <cmath>

class Vector2D
{
public:

	float x;
	float y;

	// Member Methods for vector operations
	//const means it wont change the original vector
	float CalculateMagnitude() const;
	Vector2D NormalizeVector()const;
	Vector2D SetVectorOffset(Vector2D inVectorToAdd)const;
	Vector2D ScaleVector(float inScalar)const;
	Vector2D VectorTowardsTarget(Vector2D inTargetVector)const;
	float DistanceToTarget(Vector2D inTargetedVector)const;

	// Rotation and Angle
	Vector2D Rotate(float angleRadians) const;
	float GetAngle() const;

	// Static Methods for common movements
	//static means they dont need an instance of the class to operate
	static Vector2D CosineMovement(Vector2D inCenter, float inAmplitude, float inAngle);
	static Vector2D CircularMotion(Vector2D inCenter, float inRadius, float inAngle);
	// Helper to normalize angle difference to -180 to 180 range
	static float NormalizeAngleDiff(float diff);
};
