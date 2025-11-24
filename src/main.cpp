#include <raylib.h>
#include "GameManager.h"

/*
* README---------------------------------------------------
The goal of this project is to create a game that is a mix of Snake and Troncycles, it must have good use of classes.

And needs to show of math that can be used to perform basic vector operations such as addition,
subtraction, scalar multiplication, and magnitude calculation.

The main planned classes are
Player, (Done)
Enemy, (Done)
Powerups, (not started yet)

Math/Vector2D (Implemented)
A game manager, controls all functions and updates and similar. (Done)
map editor (Done)
The Main cpp file to run the game loop (Done

There are 3 stages to start with, 
The first stage is just wall boundaries,
The Map editor will be used to create stage 2 which has some obstacles in the center of the screen.
The third stage will have procedurally generated wave like obstacles on the sides of the screen using cosine functions from the Vector2D class

There are 3 powerups to start with (one with a twist) (more to be added later?)
Speedup/down,
Split tail(Allows collison with tail to cut of the tail),
and ghost mode (collison off for a few seconds)
All powerups are just simple different colored rectangles

Player Class:
It should have a dynamic growing tail for the player
The player moves in 2 directions left and right
the Tail follows the player object

****REMOVED FOR BREVITY****
There are 3 "Bikes"/Players to start with, (More to be added later?)
Viper(Normal)
Bruiser(Slow and grows fast),
Ghost(Fast and grows slow)

Collison checks on tail, walls, and powerups (mostly done for tail and walls, powerups not started yet)

Still need to add powerups and potentially some more sounds

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