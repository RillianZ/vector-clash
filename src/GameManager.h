#pragma once
#include <vector>
#include <deque>
#include "mPlayer.h"
#include "ITailOwner.h"
#include <raylib.h>

class GameManager
{
public:
	GameManager(int screenWidth, int screenHeight);

	// Update game state (call every frame with dt)
	void Update(float dt);

	// Draw current game state (call between BeginDrawing/EndDrawing)
	void Draw();

private:
	int screenWidth_;
	int screenHeight_;
	float halfScreenWidth_;
	float halfScreenHeight_;

	// spawn invulnerability window after reset
	float spawnInvulnerableSeconds_;
	float spawnInvulnerableTimer_;

	enum class State { Menu, Running, GameOver };
	State state_;

	//number of players selected
	int selectedPlayers_;

	//resets players based on player count
	void ResetPlayers(int playerCount);

	//sets winner
	int winner_;

	// number of players at start of current match (used for game-over rules)
	int initialPlayerCount_;

	// Solo mode tracking
	float bestSoloTailLength_;

	// Players constructed here
	mPlayer playerOne_;
	mPlayer playerTwo_;
	mPlayer playerThree_;
	mPlayer playerFour_;

	// Tail owners list for generic collision checks
	//This vector holds pointers to ITailOwner objects, which represent entities with tails in the game.
	//i do not know why we use pointers here and need to look into itailowner more
	std::vector<ITailOwner*> owners_;

	// internal routines
	bool CheckHeadAgainstAllTails(ITailOwner* subject);
};
