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

//structure to hold tail hit results
struct TailHitResult
{
	//who got hit?
	ITailOwner* victim;
	//at what point of tail?
	size_t index;
};

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
	enum class gameState { Intro, Menu, Running, GameOver };
	gameState gameState_;

	//Intro state timer
	float introTimer_ = 0.0f;
	float introDuration_ = 6.0f;

	//Menu Timer and alpha
	float UpdateMenuTimer = 0.0f;
	float MenuAlpha = 0.0f;

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

	// Global spawn delay timer
	float startDelayTimer_;

	// Difficulty and hard mode timer
	enum class Difficulty { Normal, Easy, Hard, Chaos, Count };
	Difficulty difficulty_;
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

	// Private Helper Functions for State Logic
	void UpdateIntro();
	void UpdateMenu();
	void UpdateRunning(float dt);
	void UpdateGameOver();


	// Private Helper Functions for Drawing
	void DrawIntro();
	void DrawMenu();
	void DrawFadeOverlay();
	void DrawRunning();
	void DrawGameOver();

	// Private Helper Functions for UpdateRunning decomposition
	// Helper for updates
	void UpdatePowerups(float dt);

	// Handles Base Enemy + Reinforcements (Chaos Logic)
	void UpdateSoloAI(float dt);

	// Handles Deaths and Cuts
	void HandleCollisions();

	// Handles Win conditions
	void CheckGameOver();

	// Persistence Helpers
	void LoadHighScores();
	void SaveHighScores();

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

	// Bike type selection
	BikeType playerLoadouts_[4];

	// Tail collision detection helper
	TailHitResult GetTailCollisionVictim(ITailOwner* subject);

	// Check head against map obstacles
	bool CheckHeadAgainstMap(ITailOwner* subject);

	// Background music as vector
	std::vector<Music> bgmTracks_;
	int currentTrackIndex_;
	bool hasBgm_ = false;
	// Sound effects
	Sound fxMenuDing_;
	Sound fxMenuSelect_;
	Sound fxCrash_;
	Sound fxPowerup_;
	Sound fxIntroSound_;
	Sound fxSlice_;
	Sound fxSpeedup_;
	Sound fxSpeeddown_;
	Sound fxGhost_;
	Sound fxGameWinSound_;

	// Intro sound played flag
	bool introSoundPlayed_ = false;

	bool audioInitialized_ = false;
};