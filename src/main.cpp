#include <raylib.h>
#include "GameManager.h"

/*
* README---------------------------------------------------
The goal of this project is to create a game that is a mix of Snake and Troncycles, it must have good use of classes.

And needs to show of math that can be used to perform basic vector operations such as addition,
subtraction, scalar multiplication, and magnitude calculation.

I need to make sure they all have good naming, good commenting, and that the class is usuable and understandable in the future

It should also output to the console the results of the operations performed on the vectors. For Debugging

The main planned classes are
Player, (mostly done)
Enemy, (not started yet)
Powerups, (not started yet)
the powerup class will have a base class that changes the players stats on pickup and derived classes for each powerup type

Math/Vector2D, (need to update my classes to use Vector2D for  the exam)
A game manager, controls all functions and updates and similar. (Started on this one)
map editor (NOT IMPLEMENTED YET)
The Main cpp file to run the game loop (mostly done)

There are 2 stages to start with, (more to be added later?)
The first stage is just wall boundaries,
The Map editor will be used to create stage 2 which has some obstacles in the center of the screen.

There are 3 powerups to start with (one with a twist) (more to be added later?)
Speedup/down,
Split tail(Allows collison with tail to cut of the tail),
and ghost mode (collison off for a few seconds)
All powerups are just simple different colored rectangles

Player Class:
It should have a dynamic growing tail for the player
The player moves in 2 directions left and right
the Tail follows the player object

There are 3 "Bikes"/Players to start with, (More to be added later?)
Viper(Normal)
Bruiser(Slow and grows fast),
Ghost(Fast and grows slow)

Collison checks on tail, walls, and powerups (mostly done for tail and walls, powerups not started yet)

I need to add the powerups, The different player types,  then the enemy in single player, then the map generator, scoring needs to be updated for solo play , and finally polish

README END------------------------------------------------

*/

int main()
{
	int screenWidth = 1280;
	int screenHeight = 1024;

	InitWindow(screenWidth, screenHeight, "Vector Clash");
	SetTargetFPS(60);

	GameManager gm(screenWidth, screenHeight);

	while (!WindowShouldClose())
	{
		float dt = GetFrameTime();
		gm.Update(dt);

		BeginDrawing();
		ClearBackground(BLACK);
		gm.Draw();
		EndDrawing();
	}

	CloseWindow();
	return 0;
}