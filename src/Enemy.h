#pragma once
#include <string>
#include <iostream>
#include <vector>
#include "Vector2D.h"
#include "mPlayer.h"
#include <raylib.h>

class Enemy
{
public:

	Vector2D position;
	float speed = 150.f;
	float size = 5.f;
	bool isAlive{ true };

	void Respawn(int inScreenWidth, int inScreenHeight);

	void Update(Vector2D inTargetPosition);

	void Draw();
};
