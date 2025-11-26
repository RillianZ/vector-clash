#include <raylib.h>
#include "GameManager.h"

/*
****REMOVED FOR BREVITY****
* IDEA TO ADD LATER:
There are 3 "Bikes"/Players to start with, (More to be added later?)
Viper(Normal)
Bruiser(Slow and grows fast),
Ghost(Fast and grows slow)
README END------------------------------------------------

*/

int main()
{
	//Lower resolution if you have a poor persons screen
	int screenWidth = 2560;
	int screenHeight = 1440;
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