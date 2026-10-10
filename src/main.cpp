#include <raylib.h>
#include "GameManager.h"

int main()
{
	//comment out this if you wanna change resolution for testing on exam
	int screenWidth = 2560;
	int screenHeight = 1440;

	//Uncomment this for smaller screens debug for exam
	//int screenWidth = 1920;
	//int screenHeight = 1080;
	InitWindow(screenWidth, screenHeight, "Vector Clash");
	SetTargetFPS(60);
#if defined(PLATFORM_WEB)
	// [web-port] Esc would end the main loop and freeze the page, so it does nothing in the browser
	SetExitKey(KEY_NULL);
#endif

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