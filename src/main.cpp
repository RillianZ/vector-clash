#include <raylib.h>
#include "GameManager.h"

int main()
{
	//Change this to a Lower resolution if you have a poor persons screen
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