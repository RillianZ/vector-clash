#include "Enemy.h"

//For now this is just a placeholder class, but will be updated to include a basic NPC enemy.
// The enemy is a snake that uses simple math to try to go in front of the player while avoiding the sides and hinderances
//The code will be very similar to player, but with the math for moving automatically here as well.

void Enemy::Respawn(int inScreenWidth, int inScreenHeight)
{
	position = { (float)(rand() % inScreenWidth), (float)(rand() % inScreenHeight) };
}

void Enemy::Update(Vector2D inTargetPosition)
{
}

void Enemy::Draw()
{
}