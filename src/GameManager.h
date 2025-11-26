#pragma once
#include <vector>
#include <deque>
#include <fstream>
#include "mPlayer.h"
#include "ITailOwner.h"
#include <raylib.h>
#include "Enemy.h"
#include "MapGenerator.h"
#include "PowerUp.h"

class GameManager
{
public:
	// Constructor
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

	// Current game state
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
	float matchTimer_;       
	float bestSurvivalTime_; 

	//  Solo Mode Stats
	int enemiesDupedCurrent_;
	int enemiesDupedBest_;

	// Hard Mode
	bool hardMode_;
	float hardModeTimer_;

	// Spawn timer in seconds
	const float hardModeInterval_ = 5.0f; 

	// Dynamic enemies container (used to avoid memory leaks)
	std::vector<Enemy*> extraEnemies_; 

	// PowerUps
	std::vector<PowerUp> powerups_;
	float powerupSpawnTimer_;
	void SpawnPowerUp();

	// Save file name
	const std::string saveFileName_ = "vector_clash_save.txt";

	// -- Private Helper Functions for State Logic --
	void UpdateMenu();
	void UpdateRunning(float dt);
	void UpdateGameOver();

	// Persistence Helpers
	void LoadHighScores();
	void SaveHighScores();

	void DrawMenu();
	void DrawRunning();
	void DrawGameOver();

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

	// Returns pointer to collided tail owner, or nullptr if none
	ITailOwner* GetTailCollisionVictim(ITailOwner* subject);

	// Check head against map obstacles
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