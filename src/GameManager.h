#pragma once
#include <vector>
#include <deque>
#include "mPlayer.h"
#include "ITailOwner.h"
#include <raylib.h>
#include "Enemy.h"
#include "MapGenerator.h"
#include "PowerUp.h"

class GameManager
{
public:
	GameManager(int screenWidth, int screenHeight);
	// Destructor
	~GameManager();
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

	// Track player wins across matches
	std::vector<int> playerWins_;

	// Solo mode tracking
	float bestSoloTailLength_;


	//  Solo Mode Stats
	int enemiesDupedCurrent_;
	int enemiesDupedBest_;

	// Hard Mode
	bool hardMode_;
	float hardModeTimer_;
	const float hardModeInterval_ = 5.0f; // Spawn timer in seconds
	std::vector<Enemy*> extraEnemies_; // Dynamic enemies container (used to avoid memory leaks)

	// PowerUps
	std::vector<PowerUp> powerups_;
	float powerupSpawnTimer_;
	void SpawnPowerUp();

	// Players constructed here
	mPlayer playerOne_;
	mPlayer playerTwo_;
	mPlayer playerThree_;
	mPlayer playerFour_;
	// Enemy for solo mode
	Enemy enemyOne_;

	// Prediction tracking
	Vector2 previousPlayerPos_;

	// Respawn logic
	float enemyRespawnTimer_;
	// Time delay before enemy respawns after being killed
	const float enemyRespawnDelay_ = 1.0f;

	// Tail owners list for generic collision checks
	std::vector<ITailOwner*> owners_;

	// Map generator instance
	MapGenerator mapGen_;

	// Map selection
	MapType selectedMapType_;

	
	// Check head against map obstacles
	ITailOwner* GetTailCollisionVictim(ITailOwner* subject);

	bool CheckHeadAgainstMap(ITailOwner* subject);

	// Background music
	Music bgm_;
	bool hasBgm_ = false;
	// Sound effects
	Sound fxMenuDing_;
	Sound fxMenuSelect_;
	Sound fxCrash_;
	Sound fxPowerup_;

	bool audioInitialized_ = false;
};