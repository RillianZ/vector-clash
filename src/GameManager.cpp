#include "GameManager.h"
#include <raymath.h>
#include <cmath>
#include <string>
#include "Vector2D.h"

// Main Constructor
GameManager::GameManager(int screenWidth, int screenHeight)
	: screenWidth_(screenWidth) // Store screen dimensions
	, screenHeight_(screenHeight) // for use in various calculations
	, halfScreenWidth_((float)screenWidth / 2.0f) // such as centering players
	, halfScreenHeight_((float)screenHeight / 2.0f) // and enemy spawn positions
	, spawnInvulnerableSeconds_(1.f) // increase this to avoid instant self-hits at spawn
	, spawnInvulnerableTimer_(0.0f) // initialized to 0
	, gameState_(gameState::Intro) // start in Menu state
	, selectedPlayers_(1) //default to 1 for quicker testing
	, winner_(0) // no winner at start
	, bestSoloTailLength_(0.0f) // initialize best solo tail length
	, matchTimer_(0.0f) // initialize match timer
	, bestSurvivalTime_(0.0f) // initialize best survival time
	, enemiesDupedCurrent_(0)  // initialize current duped enemies
	, enemiesDupedBest_(0)     // initialize best duped enemies
	, hardModeTimer_(0.0f)     // initialize hard mode timer
	, playerOne_({ halfScreenWidth_, halfScreenHeight_ }, GetBikeStats(BikeType::Standard), KEY_LEFT, KEY_RIGHT, 180.0f, RED)
	, playerTwo_({ halfScreenWidth_, halfScreenHeight_ }, GetBikeStats(BikeType::Standard), KEY_A, KEY_D, 0.0f, ORANGE)
	, playerThree_({ halfScreenWidth_, halfScreenHeight_ }, GetBikeStats(BikeType::Standard), KEY_F, KEY_H, 270.0f, GREEN)
	, playerFour_({ halfScreenWidth_, halfScreenHeight_ }, GetBikeStats(BikeType::Standard), KEY_J, KEY_L, 90.0f, YELLOW)
	, enemyOne_({ 0, 0 }, { 16.0f, 16.0f }, 255.0f, 200.0f, BLUE, screenWidth, screenHeight) //sets enemy paramaters
	, hasBgm_(false) // bgm not loaded at start
	, audioInitialized_(false) // audio not initialized at start
	, selectedMapType_(MapType::Clean) // default to clean map
	, powerupSpawnTimer_(0.0f) // initialize powerup spawn timer
	, difficulty_(Difficulty::Normal) // default to normal difficulty
	, currentTrackIndex_(0) // initialize current track index
{
	// initialize player loadouts to Standard
	for (int i = 0; i < 4; i++)
	{
		playerLoadouts_[i] = BikeType::Standard;
	}
	// intialize player wins vector
	playerWins_ = { 0, 0, 0, 0 };

	// initialize players and owners according to selectedPlayers_
	ResetPlayers(selectedPlayers_);

	// Initialize audio device and load background music
	InitAudioDevice();
	audioInitialized_ = IsAudioDeviceReady();
	if (audioInitialized_)
	{
		// Load 3 tracks
		bgmTracks_.push_back(LoadMusicStream("resources/background.mp3"));
		bgmTracks_.push_back(LoadMusicStream("resources/background2.mp3"));
		bgmTracks_.push_back(LoadMusicStream("resources/background3.mp3"));
		bgmTracks_.push_back(LoadMusicStream("resources/background4.mp3"));
		bgmTracks_.push_back(LoadMusicStream("resources/background5.mp3"));
		bgmTracks_.push_back(LoadMusicStream("resources/background6.mp3"));
		if (!bgmTracks_.empty())
		{
			SetMusicVolume(bgmTracks_[0], 0.5f);
			PlayMusicStream(bgmTracks_[0]);
			hasBgm_ = true;
		}

		// Load sound effects
		fxMenuDing_ = LoadSound("resources/menu_ding.mp3");
		fxMenuSelect_ = LoadSound("resources/menu_select.mp3");
		fxCrash_ = LoadSound("resources/crash.mp3");
		fxPowerup_ = LoadSound("resources/powerup.mp3");
		fxIntroSound_ = LoadSound("resources/IntroSound.mp3");
		fxSlice_ = LoadSound("resources/slice.mp3");
		fxSpeedup_ = LoadSound("resources/speedup.mp3");
		fxGameWinSound_ = LoadSound("resources/game_win.mp3");
		fxSpeeddown_ = LoadSound("resources/speeddown.mp3");
		fxGhost_ = LoadSound("resources/ghost.mp3");
	}

	// Load high scores from file
	LoadHighScores();

	// Generate initial map
	mapGen_.GenerateMap(selectedMapType_, screenWidth_, screenHeight_);
}

//Player reset function
void GameManager::ResetPlayers(int playerCount)
{
	// Wipe the list clean before we add anyone
	owners_.clear();

	// Clean up old dynamic enemies from previous rounds
	for (Enemy* e : extraEnemies_) delete e;
	extraEnemies_.clear();

	// Reset constants
	const float sep = 300.0f; 

	spawnInvulnerableTimer_ = spawnInvulnerableSeconds_;
	winner_ = 0;
	matchTimer_ = 0.0f;
	enemiesDupedCurrent_ = 0;
	hardModeTimer_ = hardModeInterval_;
	enemyRespawnTimer_ = enemyRespawnDelay_;

	// Reset players and add owners
	playerOne_.Reset({ halfScreenWidth_ - sep, halfScreenHeight_ }, playerLoadouts_[0], 180.0f);
	owners_.push_back(&playerOne_);

	if (playerCount >= 2) {
		playerTwo_.Reset({ halfScreenWidth_ + sep, halfScreenHeight_ }, playerLoadouts_[1], 0.0f);
		owners_.push_back(&playerTwo_);
	}
	if (playerCount >= 3) {
		playerThree_.Reset({ halfScreenWidth_, halfScreenHeight_ - sep }, playerLoadouts_[2], 270.0f);
		owners_.push_back(&playerThree_);
	}
	if (playerCount >= 4) {
		playerFour_.Reset({ halfScreenWidth_, halfScreenHeight_ + sep }, playerLoadouts_[3], 90.0f);
		owners_.push_back(&playerFour_);
	}

	// Solo mode enemy setup
	if (playerCount == 1)
	{
		// Base Enemy Setup
		float baseEnemySpeed = 255.0f;
		if (difficulty_ == Difficulty::Easy) baseEnemySpeed = 200.0f;
		else if (difficulty_ == Difficulty::Hard) baseEnemySpeed = 300.0f;
		else if (difficulty_ == Difficulty::Chaos) baseEnemySpeed = (float)GetRandomValue(250, 400);

		enemyOne_.Reset({ halfScreenWidth_ + sep, halfScreenHeight_ });
		enemyOne_.SetSpeed(baseEnemySpeed);
		enemyOne_.SetPlayerReference(&playerOne_);
		enemyOne_.SetObstaclesReference(&mapGen_.GetObstacles());

		// Add to owners list
		owners_.push_back(&enemyOne_);

		// Spawn Extra Enemies (Hard/Chaos)
		int extraCount = 0;
		if (difficulty_ == Difficulty::Hard) extraCount = 1;
		else if (difficulty_ == Difficulty::Chaos) extraCount = GetRandomValue(2, 4);

		for (int i = 0; i < extraCount; i++)
		{
			float speed = 255.0f;
			if (difficulty_ == Difficulty::Hard) speed = 280.0f;
			if (difficulty_ == Difficulty::Chaos) speed = (float)GetRandomValue(180, 320);

			// Random spawn position away from edges
			Vector2 spawnPos = { (float)GetRandomValue(100, screenWidth_ - 100), (float)GetRandomValue(100, screenHeight_ - 100) };
			Color c = ColorFromHSV((float)GetRandomValue(0, 360), 0.8f, 0.9f);

			// Create the new enemy
			Enemy* newEnemy = new Enemy(spawnPos, { 16.0f, 16.0f }, speed, 200.0f, c, screenWidth_, screenHeight_);

			// Configure refs
			newEnemy->SetPlayerReference(&playerOne_);
			newEnemy->SetObstaclesReference(&mapGen_.GetObstacles());
			newEnemy->SetSpeed(speed); 

			// Store in lists
			extraEnemies_.push_back(newEnemy); 
			owners_.push_back(newEnemy);       
		}
	}

	// Final Tracking
	previousPlayerPos_ = playerOne_.GetPosition();
	initialPlayerCount_ = playerCount;

	// Reset powerups
	powerups_.clear();
	powerupSpawnTimer_ = (float)GetRandomValue(2, 5);
}

// Spawns a power-up at a random valid location
void GameManager::SpawnPowerUp()
{
	int safePadding = 50;
	Vector2D pos;
	bool valid = false;
	int attempts = 0;

	// Try to find a spot not inside a wall
	while (!valid && attempts < 10)
	{
		pos = { (float)GetRandomValue(safePadding, screenWidth_ - safePadding),
				(float)GetRandomValue(safePadding, screenHeight_ - safePadding) };

		// Check against map obstacles
		bool hitWall = false;
		for (const auto& wall : mapGen_.GetObstacles())
		{
			if (CheckCollisionPointRec({ pos.x, pos.y }, wall)) hitWall = true;
		}
		if (!hitWall) valid = true;
		attempts++;
	}

	if (valid)
	{
		PowerUpType type = (PowerUpType)GetRandomValue(0, 3);
		powerups_.push_back(PowerUp(pos, type));
	}
}

// Check head of subject against all tails, returns first hit found
TailHitResult GameManager::GetTailCollisionVictim(ITailOwner* subject)
{
	if (!subject->IsAlive()) return { nullptr, 0 };

	// Get Head Properties
	Vector2 headCenterRay = subject->GetPosition();
	Vector2 headSize = subject->GetSize();
	Vector2D headVec = { headCenterRay.x, headCenterRay.y };
	float headRadius = std::max(headSize.x, headSize.y) * 0.5f;

	// Cycle through all owners
	for (ITailOwner* owner : owners_)
	{
		// Skip self and dead owners
		if (!owner->IsAlive()) continue;
		const auto& tail = owner->GetTailSegments();
		if (tail.empty()) continue;

		// Determine how many tail segments to skip at the end
		size_t skipLast = (owner == subject) ? 20 : 0;
		size_t limit = tail.size();
		if (limit > skipLast) limit -= skipLast;
		else limit = 0;

		// Check each tail segment for collision
		for (size_t i = 0; i < limit; ++i)
		{
			Vector2D segVec = { tail[i].x, tail[i].y };

			// STRICT Vector2D Constraint used here
			if (headVec.DistanceToTarget(segVec) < (headRadius + 5.0f))
			{
				//return owner and index of hit segment
				return { owner, i };
			}
		}
	}

	return { nullptr, 0 };
}

// Check head of subject against map obstacles
bool GameManager::CheckHeadAgainstMap(ITailOwner* subject)
{
	if (!subject->IsAlive()) return false;

	// Get Head Properties
	Vector2 headPos = subject->GetPosition();
	Vector2 headSize = subject->GetSize();
	float headRadius = std::max(headSize.x, headSize.y) * 0.5f;

	// cycle through obstacles provided by MapGenerator
	const std::vector<Rectangle>& obstacles = mapGen_.GetObstacles();
	// Check each wall for collision
	for (const auto& wall : obstacles)
	{
		// Standard Raylib collision: Circle (Head) vs Rectangle (Wall)
		if (CheckCollisionCircleRec(headPos, headRadius, wall))
		{
			return true;
		}
	}
	return false;
}

// Handle all collision detection and resolution
void GameManager::HandleCollisions()
{
	std::vector<ITailOwner*> pendingDeaths;

	for (ITailOwner* o : owners_)
	{
		if (!o->IsAlive()) continue;
		// Check if the owner is a player and if they are in ghost mode
		mPlayer* player = dynamic_cast<mPlayer*>(o);
		bool isGhost = (player && player->IsPhasing());
		bool markedForDeath = false;

		// Wall Checks
		if (CheckHeadAgainstMap(o)) markedForDeath = true;

		// Tail Checks (Updated)
		TailHitResult hit = GetTailCollisionVictim(o); 

		// Process Tail Hit Result
		if (hit.victim && !markedForDeath)
		{
			if (isGhost) { /* Pass through */ }
			else if (player && player->HasTailCutter())
			{
				// New Cutter Logic
				player->ConsumeTailCutter();

				// Define clearance buffer to avoid immediate re-collision
				size_t clearanceBuffer = 12;
				size_t cutIndex = hit.index + clearanceBuffer;

				// Safety Clamp
				size_t maxIndex = hit.victim->GetTailSegments().size();
				if (maxIndex > 0)
				{
					// Ensure we don't cut beyond the tail length
					if (cutIndex >= maxIndex) cutIndex = maxIndex - 1;

					// Perform the cut
					hit.victim->CutTailAtIndex(cutIndex);
				}

				// Play sound effect
				if (audioInitialized_) PlaySound(fxSlice_); 
			}
			else
			{
				// no powerup = death
				markedForDeath = true;
			}
		}

		if (markedForDeath) pendingDeaths.push_back(o);
	}

	// Execute Deaths
	bool someoneDied = false;
	for (ITailOwner* victim : pendingDeaths)
	{
		if (victim->IsAlive())
		{
			victim->OwnerKill();
			if (selectedPlayers_ == 1 && victim != &playerOne_) enemiesDupedCurrent_++;
			someoneDied = true;
		}
	}
	if (someoneDied && audioInitialized_) PlaySound(fxCrash_);
}

// Main per-frame update function
void GameManager::Update(float dt)
{
	// Global: countdown spawn invul timer if active
	if (spawnInvulnerableTimer_ > 0.0f) spawnInvulnerableTimer_ -= dt;

	// Delegate to helpers via state machine
	switch (gameState_)
	{
	case gameState::Intro:
		UpdateIntro();
		break;
	case gameState::Menu:
		UpdateMenu();
		break;

	case gameState::Running:
		UpdateRunning(dt);
		break;

	case gameState::GameOver:
		UpdateGameOver();
		break;
	}
}

// Helper Menu Logic
void GameManager::UpdateMenu()
{
	bool changed = false;
	UpdateMenuTimer += GetFrameTime();
	float fadeDuration = 2.0f;

	// Calculate Alpha (0.0 to 1.0)
	if (UpdateMenuTimer < fadeDuration)
	{
		MenuAlpha = UpdateMenuTimer / fadeDuration;
	}
	else
	{
		MenuAlpha = 1.0f;
	}



	// PLAYER BIKE SELECTION 
	// Helper Lambda to cycle bikes safely
	auto CycleBike = [&](int playerIndex, int direction)
		{
			int current = (int)playerLoadouts_[playerIndex];
			int count = (int)BikeType::Count;

			current += direction;

			// Wrap around logic
			if (current < 0) current = count - 1;
			else if (current >= count) current = 0;

			playerLoadouts_[playerIndex] = (BikeType)current;
			changed = true;
		};

	// Player 1 (Arrows)
	if (IsKeyPressed(KEY_LEFT))  CycleBike(0, -1);
	if (IsKeyPressed(KEY_RIGHT)) CycleBike(0, 1);

	// Player 2 (A / D)
	if (IsKeyPressed(KEY_A)) CycleBike(1, -1);
	if (IsKeyPressed(KEY_D)) CycleBike(1, 1);

	// Player 3 (F / H)
	if (IsKeyPressed(KEY_F)) CycleBike(2, -1);
	if (IsKeyPressed(KEY_H)) CycleBike(2, 1);

	// Player 4 (J / L)
	if (IsKeyPressed(KEY_J)) CycleBike(3, -1);
	if (IsKeyPressed(KEY_L)) CycleBike(3, 1);

	// Player Count (Up / Down)
	if (IsKeyPressed(KEY_UP))
	{
		selectedPlayers_ = std::min(4, selectedPlayers_ + 1);
		changed = true;
	}
	if (IsKeyPressed(KEY_DOWN))
	{
		selectedPlayers_ = std::max(1, selectedPlayers_ - 1);
		changed = true;
	}

	// Map Selection (M Key), cycles forward and shows immediate preview
	if (IsKeyPressed(KEY_M))
	{
		int type = (int)selectedMapType_ + 1;
		if (type >= (int)MapType::Count) type = 0;

		selectedMapType_ = (MapType)type;
		mapGen_.GenerateMap(selectedMapType_, screenWidth_, screenHeight_);
		changed = true;
	}

	// Input Toggle difficulty (TAB Key)
	if (IsKeyPressed(KEY_TAB)) 
	{
		int d = (int)difficulty_;
		d++;
		if (d >= (int)Difficulty::Count) d = 0; // Wrap: Normal -> Easy -> Hard -> Chaos -> Normal
		difficulty_ = (Difficulty)d;
		changed = true;
	}

	// CHANGE MUSIC TRACK (T)
	if (IsKeyPressed(KEY_T) && hasBgm_)
	{
		// Stop current
		StopMusicStream(bgmTracks_[currentTrackIndex_]);

		// Increment Index
		currentTrackIndex_++;
		if (currentTrackIndex_ >= bgmTracks_.size()) currentTrackIndex_ = 0;

		// Play new
		SetMusicVolume(bgmTracks_[currentTrackIndex_], 0.5f);
		PlayMusicStream(bgmTracks_[currentTrackIndex_]);

		// Play sound effect
		PlaySound(fxMenuDing_);
	}

	// Play Sound on Change
	if (changed && audioInitialized_) PlaySound(fxMenuDing_);

	// Start Game
	if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
	{
		if (audioInitialized_) PlaySound(fxMenuSelect_);
		ResetPlayers(selectedPlayers_);

		mapGen_.GenerateMap(selectedMapType_, screenWidth_, screenHeight_);

		spawnInvulnerableTimer_ = spawnInvulnerableSeconds_;
		gameState_ = gameState::Running;
	}

	// Audio Stream Update
	if (audioInitialized_ && hasBgm_ && !bgmTracks_.empty())
	{
		UpdateMusicStream(bgmTracks_[currentTrackIndex_]);
	}
}

// Helper Game Over Logic
void GameManager::UpdateGameOver()
{
	if (IsKeyPressed(KEY_SPACE))
	{
		ResetPlayers(selectedPlayers_);
		
		mapGen_.GenerateMap(selectedMapType_, screenWidth_, screenHeight_);

		gameState_ = gameState::Running;
		winner_ = 0;
	}
	else if (IsKeyPressed(KEY_ENTER))
	{
		gameState_ = gameState::Menu;
		UpdateMenuTimer = 0.0f;
		MenuAlpha = 0.0f;
	}

	// Keep music playing at lower volume
	if (audioInitialized_ && hasBgm_ && !bgmTracks_.empty())
	{
		UpdateMusicStream(bgmTracks_[currentTrackIndex_]);
		SetMusicVolume(bgmTracks_[currentTrackIndex_], 0.5f);
	}
		
}

// Helper RunningState Logic 
void GameManager::UpdateRunning(float dt)
{
	// Stats Tracking (Solo)
	if (selectedPlayers_ == 1 && playerOne_.IsAlive())
	{
		matchTimer_ += dt;
		if (matchTimer_ > bestSurvivalTime_) bestSurvivalTime_ = matchTimer_;
	}

	// Sub-Systems
	UpdatePowerups(dt);
	UpdateSoloAI(dt); 

	// Update All Entities (Movement/Steering)
	for (ITailOwner* o : owners_) o->OwnerUpdate(dt);

	// Global Solo Stats (Best Tail)
	if (selectedPlayers_ == 1 && !owners_.empty())
	{
		float cur = owners_[0]->GetTailPixelLength();
		if (cur > bestSoloTailLength_) bestSoloTailLength_ = cur;
		if (enemiesDupedCurrent_ > enemiesDupedBest_) enemiesDupedBest_ = enemiesDupedCurrent_;
	}

	// Physics & State
	if (spawnInvulnerableTimer_ <= 0.0f)
	{
		HandleCollisions();
	}

	// Check Win Conditions
	CheckGameOver();

	// Audio
	if (audioInitialized_ && hasBgm_ && !bgmTracks_.empty())
	{
		UpdateMusicStream(bgmTracks_[currentTrackIndex_]);
		SetMusicVolume(bgmTracks_[currentTrackIndex_], 1.0f);
	}
		
}

// Update power-up system
void GameManager::UpdatePowerups(float dt)
{
	// Spawning Logic
	powerupSpawnTimer_ -= dt;
	if (powerupSpawnTimer_ <= 0.0f)
	{
		size_t maxPowerups = 15;

		if (powerups_.size() < maxPowerups)
		{
		SpawnPowerUp();
		// Chaos Mode: 50% chance for double spawn
		// Check size again to strictly enforce the limit
		if (difficulty_ == Difficulty::Chaos &&	powerups_.size() < maxPowerups && GetRandomValue(0, 100) > 50)
		{
			SpawnPowerUp();
		}
	}

		// Reset Timer
		float minTime = 5.0f, maxTime = 10.0f;
		switch (difficulty_)
		{
		case Difficulty::Easy:   minTime = 3.0f; maxTime = 6.0f; break;
		case Difficulty::Hard:   minTime = 8.0f; maxTime = 12.0f; break;
		case Difficulty::Chaos:  minTime = 1.0f; maxTime = 3.0f; break;
		default:                 minTime = 5.0f; maxTime = 7.0f; break;
		}
		powerupSpawnTimer_ = (float)GetRandomValue((int)minTime, (int)maxTime);
	}

	// Update Animations
	for (auto& p : powerups_) p.Update(dt);

	// Collision Logic
	for (ITailOwner* o : owners_)
	{
		if (!o->IsAlive()) continue;
		mPlayer* player = dynamic_cast<mPlayer*>(o);

		if (player)
		{
			for (auto& pup : powerups_)
			{
				if (pup.IsActive() && CheckCollisionRecs(player->GetRect(), pup.GetRect()))
				{
					player->ApplyPowerUp(pup.GetType());
					pup.Deactivate();
					if (audioInitialized_)
					{
						if (pup.GetType() == PowerUpType::SpeedBoost)
						{
							// Speed boost specific sound
							PlaySound(fxSpeedup_);
						}
						else if (pup.GetType() == PowerUpType::SlowDown)
						{
							// Slow down sound
							PlaySound(fxSpeeddown_);
						}
						else if (pup.GetType() == PowerUpType::PhaseShift)
						{
							// Ghost mode sound
							PlaySound(fxGhost_);
						}
						else 
						{ 
							// Other power-ups
							PlaySound(fxPowerup_); 
						}
					}
				}
			}
		}
	}

	// Cleanup
	powerups_.erase(std::remove_if(powerups_.begin(), powerups_.end(),
		[](const PowerUp& p) { return !p.IsActive(); }), powerups_.end());
}

// Update Solo AI Logic
void GameManager::UpdateSoloAI(float dt)
{
	if (selectedPlayers_ != 1) return;

	// Base Enemy Respawn
	if (!enemyOne_.IsAlive())
	{
		enemyRespawnTimer_ -= dt;
		if (enemyRespawnTimer_ <= 0.0f)
		{
			// Reset to safe position
			int safeMinX = (int)(screenWidth_ * 0.2f);
			int safeMaxX = (int)(screenWidth_ * 0.8f);
			Vector2 spawnPos;
			int attempts = 0;
			do {
				spawnPos = { (float)GetRandomValue(safeMinX, safeMaxX), (float)GetRandomValue(50, screenHeight_ - 50) };
				attempts++;
			} while (Vector2Distance(spawnPos, playerOne_.GetPosition()) < 400.0f && attempts < 10);

			enemyOne_.Reset(spawnPos);
			enemyRespawnTimer_ = enemyRespawnDelay_;
		}
	}
	// Base Enemy Prediction AI
	else if (playerOne_.IsAlive())
	{
		Vector2D currPos = { playerOne_.GetPosition().x, playerOne_.GetPosition().y };
		Vector2D prevPos = { previousPlayerPos_.x, previousPlayerPos_.y };
		Vector2D vel = prevPos.VectorTowardsTarget(currPos);
		Vector2D aim = currPos.SetVectorOffset(vel.ScaleVector(35.0f));
		enemyOne_.SetTarget({ aim.x, aim.y });
	}
	else
	{
		enemyOne_.SetTarget({ halfScreenWidth_, halfScreenHeight_ });
	}
	previousPlayerPos_ = playerOne_.GetPosition();

	// Reinforcement Logic (Hard / Chaos)
	if ((difficulty_ == Difficulty::Hard || difficulty_ == Difficulty::Chaos) && playerOne_.IsAlive())
	{
		hardModeTimer_ -= dt;
		if (hardModeTimer_ <= 0.0f)
		{
			// CAP LOGIC 
			int maxEnemies = 2; // Hard Mode Limit
			if (difficulty_ == Difficulty::Chaos) maxEnemies = 10; // Chaos Limit

			if (extraEnemies_.size() < maxEnemies)
			{
				// Determine Logic
				hardModeTimer_ = (difficulty_ == Difficulty::Chaos) ? 2.0f : 5.0f;

				// Chaos Randomization
				float spawnSpeed = 260.0f;
				float spawnTurn = 210.0f;

				if (difficulty_ == Difficulty::Chaos) {
					spawnSpeed = (float)GetRandomValue(180, 350);
					spawnTurn = spawnSpeed * 0.8f;
				}

				// Spawn
				int safeMinX = (int)(screenWidth_ * 0.2f);
				int safeMaxX = (int)(screenWidth_ * 0.8f);
				Vector2 spawnPos = { (float)GetRandomValue(safeMinX, safeMaxX), (float)GetRandomValue(50, screenHeight_ - 50) };
				Color c = ColorFromHSV((float)GetRandomValue(0, 360), 0.8f, 0.9f);

				// Used spawnSpeed/spawnTurn variables instead of hardcoded 260/210
				Enemy* newEnemy = new Enemy(spawnPos, { 16.0f, 16.0f }, spawnSpeed, spawnTurn, c, screenWidth_, screenHeight_);

				newEnemy->SetPlayerReference(&playerOne_);
				newEnemy->SetObstaclesReference(&mapGen_.GetObstacles());
				newEnemy->SetSpeed(spawnSpeed); // Ensure internal state matches

				extraEnemies_.push_back(newEnemy);
				owners_.push_back(newEnemy);
			}
			else
			{
				// Cap reached, wait a bit before checking again
				hardModeTimer_ = 1.0f;
			}
		}
	}

	// Update Extra Enemy AI & Cleanup
	for (size_t i = 0; i < extraEnemies_.size(); )
	{
		Enemy* e = extraEnemies_[i];
		if (e->IsAlive())
		{
			// Simple seek player
			e->SetTarget(playerOne_.GetPosition());
			i++;
		}
		else
		{
			// Remove dead enemy
			ITailOwner* deadPtr = e;
			owners_.erase(std::remove(owners_.begin(), owners_.end(), deadPtr), owners_.end());
			delete e;
			extraEnemies_.erase(extraEnemies_.begin() + i);
		}
	}
}

// Update intro screen
void GameManager::UpdateIntro() {
	introTimer_ += GetFrameTime();
	// Play intro sound once at start
	if (introTimer_ >= 2.0f && !introSoundPlayed_ && audioInitialized_) {
		PlaySound(fxIntroSound_);
		introSoundPlayed_ = true;
	}
	if (introTimer_ > introDuration_) {
		gameState_ = gameState::Menu;
		UpdateMenuTimer = 0.0f;
		MenuAlpha = 0.0f;
	}
}

//MAJOR DRAWING BLOCK
// Draw current game state (call between BeginDrawing/EndDrawing)
void GameManager::Draw()
{
	switch (gameState_)
	{
	case gameState::Intro:
		DrawIntro();
		break;

	case gameState::Menu:
		DrawMenu();
		break;

	case gameState::Running:
		DrawRunning();
		break;

	case gameState::GameOver:
		DrawGameOver();
		break;
	}
}

// Helper Draw Menu
void GameManager::DrawMenu()
{
	// Determine Grid Style based on Difficulty
	Color gridColor = Fade(DARKGRAY, 0.1f); // Default (Normal/Easy)
	bool pulse = false;

	if (difficulty_ == Difficulty::Hard)
	{
		// Hard mode = static red
		gridColor = Fade(RED, 0.15f);
		pulse = false;
	}
	else if (difficulty_ == Difficulty::Chaos)
	{
		// Chaos mode =  Pulsating Dark red
		gridColor = MAROON;
		pulse = true;
	}

	// Draw the map with the configured style
	mapGen_.Draw(gridColor, pulse);

	Color dimColor = Fade(BLACK, 0.85f); // Default

	if (difficulty_ == Difficulty::Hard)
	{
		dimColor = Fade(MAROON, 0.6f); // Reddish tint
	}
	else if (difficulty_ == Difficulty::Chaos)
	{
		dimColor = Fade(DARKBLUE, 0.6f); // Blueish tint
	}
	else if (difficulty_ == Difficulty::Easy)
	{
		dimColor = Fade(GREEN, 0.6f); // greenish tint
	}

	DrawRectangle(0, 0, screenWidth_, screenHeight_, dimColor);

	// Draw High Scores (Top Corners)
	std::string scoreLen = "BEST LEN: " + std::to_string((int)bestSoloTailLength_);
	std::string scoreTime = "BEST TIME: " + std::to_string((int)bestSurvivalTime_) + "s";

	DrawText(scoreLen.c_str(), 20, 20, 20, LIME);
	DrawText(scoreTime.c_str(), screenWidth_ - MeasureText(scoreTime.c_str(), 20) - 20, 20, 20, SKYBLUE);

	// Draw Music Hint
	DrawText("Change Background Music: 'T'", 100, screenHeight_ - 100, 50, MAROON);

	// Info text positions
	int infoY = 200;

	const char* diffText = "DIFFICULTY: NORMAL";
	Color diffColor = GRAY;

	switch (difficulty_)
	{
	case Difficulty::Easy:
		diffText = "DIFFICULTY: EASY";
		diffColor = GREEN;
		break;
	case Difficulty::Normal:
		diffText = "DIFFICULTY: NORMAL";
		diffColor = RAYWHITE;
		break;
	case Difficulty::Hard:
		diffText = "DIFFICULTY: HARD";
		diffColor = ORANGE;
		break;
	case Difficulty::Chaos:
		diffText = "DIFFICULTY: CHAOS";
		diffColor = PURPLE; 
		break;
	}

	DrawText(diffText, (screenWidth_ - MeasureText(diffText, 30)) / 2, 130, 30, diffColor);
	DrawText("(PRESS TAB)", (screenWidth_ - MeasureText("(PRESS TAB)", 30)) / 2, 160, 30, DARKGRAY);


	// Title
	DrawText("VECTOR CLASH", (screenWidth_ - MeasureText("VECTOR CLASH", 80)) / 2, 50, 80, MAROON);

	// Player count info
	std::string countText = "PLAYERS: < " + std::to_string(selectedPlayers_) + " >  (UP/DOWN)";
	int countX = (screenWidth_ - MeasureText(countText.c_str(), 30)) / 2;
	DrawText(countText.c_str(), countX, infoY, 30, YELLOW);

	// draw selection of players
	std::string sel = "Players: " + std::to_string(selectedPlayers_);
	int sfs = 48;
	int sw = MeasureText(sel.c_str(), sfs);
	DrawText(sel.c_str(), (screenWidth_ - sw) / 2, 280, sfs, YELLOW);

	// draw selection of map type
	std::string mapName;
	switch (selectedMapType_)
	{
	case MapType::Clean:mapName = "Map: CLEAN";	break;
	case MapType::Fixed:mapName = "Map: FIXED";	break;
	case MapType::Wave:	mapName = "Map: WAVE";	break;
	case MapType::Donut:mapName = "Map: Donut";	break;
	case MapType::Grid:	mapName = "Map: GRID";	break;
	default: mapName = "UNKNOWN"; break;
	}

	// Map Description text
	std::string mapText = "MAP: " + mapName + "  (PRESS 'M')";
	int mapX = (screenWidth_ - MeasureText(mapText.c_str(), 30)) / 2;
	DrawText(mapText.c_str(), mapX, infoY + 40, 30, LIME);

	// Player Slots Layout Configuration
	int boxWidth = 300;
	int boxHeight = 250;
	int gap = 40;        
	int startY = 350;    

	// Total Width = (4 boxes) + (3 gaps between them)
	int totalGroupWidth = (4 * boxWidth) + (3 * gap);

	// Ensures centering
	int startX = (screenWidth_ - totalGroupWidth) / 2;

	// Helper Arrays to avoid big switch statements
	Color idColors[4] = { RED, ORANGE, GREEN, YELLOW };
	const char* keys[4] = { "LEFT / RIGHT", "A / D", "F / H", "J / L" };

	for (int i = 0; i < 4; i++)
	{
		// Calculate box position
		int x = startX + (i * (boxWidth + gap));
		int y = startY;

		// Determine if this player slot is active
		bool isActive = (i < selectedPlayers_);

		// Draw The Border Identity
		// If active, color is solid, else faded
		Color borderColor = isActive ? idColors[i] : Fade(idColors[i], 0.3f);

		// Draw the outline of the player box
		DrawRectangleLinesEx({ (float)x, (float)y, (float)boxWidth, (float)boxHeight }, 4.0f, borderColor);

		// Draw the Player Label
		std::string label = "P" + std::to_string(i + 1);
		DrawText(label.c_str(), x + 10, y + 10, 20, borderColor);

		// Draw The Content for Active / Inactive States
		if (isActive)
		{
			// Get the Data for this Player's selected bike
			BikeType type = playerLoadouts_[i];
			BikeConfig stats = GetBikeStats(type); 

			// Draw Class Name (Centered)
			std::string className;
			if (type == BikeType::Standard) className = "STANDARD";
			else if (type == BikeType::Viper) className = "VIPER";
			else if (type == BikeType::Bruiser) className = "BRUISER";
			else if (type == BikeType::Ghost) className = "GHOST";

			// Center the text inside the box
			int nameW = MeasureText(className.c_str(), 30);
			DrawText(className.c_str(), x + (boxWidth - nameW) / 2, y + 50, 30, stats.color);

			// Draw The Visual Representation
			int previewSize = 60;

			// Center the square in the box
			int px = x + (boxWidth - previewSize) / 2;
			int py = y + 100;

			// Draw Identity Border
			DrawRectangle(px - 3, py - 3, previewSize + 6, previewSize + 6, idColors[i]);
			// Draw Class Fill
			DrawRectangle(px, py, previewSize, previewSize, stats.color);

			// Draw Controls
			int keyW = MeasureText(keys[i], 20);
			DrawText(keys[i], x + (boxWidth - keyW) / 2, y + boxHeight - 40, 20, LIGHTGRAY);
		}
		else
		{
			// INACTIVE STATE
			const char* txt = "OFF";
			int txtW = MeasureText(txt, 40);
			// Center the "OFF" text
			DrawText(txt, x + (boxWidth - txtW) / 2, y + 100, 40, DARKGRAY);
		}
	}

	// Powerup Legend 
	int legY = 750;
	int pSize = 30; 
	int legGap = 300;

	// Calculate total width to center the legend
	int legendStartX = (screenWidth_ - (3 * legGap)) / 2;

	DrawText("POWERUPS:", (screenWidth_ - MeasureText("POWERUPS:", 20)) / 2, legY - 50, 20, LIGHTGRAY);

	// Helper Lambda to draw each legend item
	auto DrawLegendItem = [&](int x, Color c, const char* text) {
		DrawRectangle(x - (pSize / 2), legY, pSize, pSize, c);
		DrawRectangleLines(x - (pSize / 2), legY, pSize, pSize, RAYWHITE);
		DrawText(text, x + 25, legY + 5, 20, RAYWHITE);
		};

	// Speed (Green)
	DrawLegendItem(legendStartX, GREEN, "Speed Boost");

	// Slow (Lime)
	DrawLegendItem(legendStartX + legGap, LIME, "Slow Down");

	// Ghost (Blue)
	DrawLegendItem(legendStartX + (legGap * 2), SKYBLUE, "Ghost Mode");

	// Cutter (Gold)
	DrawLegendItem(legendStartX + (legGap * 3), GOLD, "Tail Cutter");


	// Bottom Instructions (Press Start)
	const char* instr = "PRESS [ENTER]/[SPACE] TO START";
	// Pulse effect
	Color startColor = (int)(GetTime() * 2) % 2 == 0 ? RAYWHITE : GRAY;
	DrawText(instr, (screenWidth_ - MeasureText(instr, 30)) / 2, screenHeight_ - 400, 30, startColor);

	DrawFadeOverlay();

}

//helper to fadein menu
void GameManager::DrawFadeOverlay()
{
	// If MenuAlpha is 1.0, the fade is complete, so we don't draw the rectangle
	if (MenuAlpha < 1.0f)
	{
		// Inverse the alpha: Starts at 1.0 (Opaque Black), ends at 0.0 (Transparent)
		float overlayAlpha = 1.0f - MenuAlpha;

		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, overlayAlpha));
	}
}

// Helper Draw Running Game
void GameManager::DrawRunning()
{
	// Determine Grid Style based on Difficulty
	Color gridColor = Fade(DARKGRAY, 0.1f); // Default (Normal/Easy)
	bool pulse = false;

	if (difficulty_ == Difficulty::Hard)
	{
		// Hard mode = static red
		gridColor = Fade(RED, 0.15f);
		pulse = false;
	}
	else if (difficulty_ == Difficulty::Chaos)
	{
		// Chaos mode = Pulsating Maroon
		gridColor =MAROON;
		pulse = true;
	}

	// Draw the map with the configured style
	mapGen_.Draw(gridColor, pulse);

	// Draw power-ups
	for (auto& p : powerups_) p.Draw();

	// Draw entities (Players + Enemies)
	for (ITailOwner* o : owners_) o->OwnerDraw();

	// HUD / Debug Overlay
	// Show duped count in HUD for Solo
	if (selectedPlayers_ == 1)
	{
		std::string duped = "Duped: " + std::to_string(enemiesDupedCurrent_);
		DrawText(duped.c_str(), screenWidth_ - 150, 10, 30, ORANGE);

		std::string dbg = "TailPts: " + std::to_string(playerOne_.GetTailSegments().size());
		DrawText(dbg.c_str(), 10, 10, 30, WHITE);

		std::string pupTimer = "PowerUp in: " + std::to_string((int)powerupSpawnTimer_);
		DrawText(pupTimer.c_str(), 10, 80, 20, YELLOW);

		std::string activeCount = "Active P-Ups: " + std::to_string(powerups_.size());
		DrawText(activeCount.c_str(), 10, 100, 20, YELLOW);

		if (difficulty_ == Difficulty::Hard || difficulty_ == Difficulty::Chaos)
		{
			std::string label = "Reinforcements: ";
			Color textColor = RED;

			if (difficulty_ == Difficulty::Chaos)
			{
				label = "CHAOS SPAWN: ";
				textColor = PURPLE;
			}

			std::string spawnT = label + std::to_string((int)hardModeTimer_);

			// Center the text roughly
			DrawText(spawnT.c_str(), screenWidth_ / 2 - 220, 10, 30, textColor);
		}
	}
}

// Helper Draw GameOver
void GameManager::DrawGameOver()
{
	// draw messages for solo
	if (selectedPlayers_ == 1)
	{
		//Display "Game Over" message
		float finalLen = owners_.empty() ? 0.0f : owners_[0]->GetTailPixelLength();
		std::string msg = "Solo run finished";
		int fontSize = 40;
		int textWidth = MeasureText(msg.c_str(), fontSize);
		DrawText(msg.c_str(), (screenWidth_ - textWidth) / 2, screenHeight_ / 2 - 100, fontSize, RED);

		//Display length score
		char buf[128];
		snprintf(buf, sizeof(buf), "Length: %.0f px (Best: %.0f)", finalLen, bestSoloTailLength_);
		DrawText(buf, (screenWidth_ - MeasureText(buf, 30)) / 2, screenHeight_ / 2 - 50, 30, BLUE);

		//Display time score
		snprintf(buf, sizeof(buf), "Time Alive: %.2fs (Best: %.2fs)", matchTimer_, bestSurvivalTime_);
		DrawText(buf, (screenWidth_ - MeasureText(buf, 30)) / 2, screenHeight_ / 2 - 10, 30, RAYWHITE);

		//Display duped score
		snprintf(buf, sizeof(buf), "Enemies Duped: %d (Best: %d)", enemiesDupedCurrent_, enemiesDupedBest_);
		DrawText(buf, (screenWidth_ - MeasureText(buf, 30)) / 2, screenHeight_ / 2 + 30, 30, ORANGE);

		// Hint to restart or menu
		std::string hint = "Space = Retry, Enter = Menu";
		int hintSize = 20;
		int hintW = MeasureText(hint.c_str(), hintSize);
		DrawText(hint.c_str(), (screenWidth_ - hintW) / 2, screenHeight_ / 2 + 60, hintSize, LIGHTGRAY);
	}
	else // draw messages for Multiplayer
	{
		std::string msg;
		if (winner_ == 1) msg = "PLAYER 1 WINS!";
		else if (winner_ == 2) msg = "PLAYER 2 WINS!";
		else if (winner_ == 3) msg = "PLAYER 3 WINS!";
		else if (winner_ == 4) msg = "PLAYER 4 WINS!";
		else msg = "DRAW GAME!"; // Tie handling

		int fontSize = 50;
		int textWidth = MeasureText(msg.c_str(), fontSize);
		DrawText(msg.c_str(), (screenWidth_ - textWidth) / 2, screenHeight_ / 2 - 100, fontSize, GOLD);

		// Draw Scoreboard
		int scoreY = screenHeight_ / 2 + 100;
		int scoreGap = 200;
		int totalScoreW = selectedPlayers_ * scoreGap;
		int scoreX = (screenWidth_ - totalScoreW) / 2;

		DrawText("SCOREBOARD", (screenWidth_ - MeasureText("SCOREBOARD", 30)) / 2, scoreY - 40, 30, LIGHTGRAY);

		for (int i = 0; i < selectedPlayers_; i++)
		{
			Color pColor = WHITE;
			if (i == 0) pColor = RED;
			else if (i == 1) pColor = ORANGE;
			else if (i == 2) pColor = GREEN;
			else if (i == 3) pColor = YELLOW;

			std::string s = "P" + std::to_string(i + 1) + ": " + std::to_string(playerWins_[i]);

			//Centering math
			int textWidth = MeasureText(s.c_str(), 30);

			//Find the left edge of this specific column
			int columnLeft = scoreX + (i * scoreGap);

			// Calculate the centered X position within this column
			int centeredX = columnLeft + (scoreGap - textWidth) / 2;

			DrawText(s.c_str(), centeredX, scoreY, 30, pColor);
		}

		std::string hint = "Press Space to restart, Enter to return to Menu";
		int hintSize = 28;
		int hintW = MeasureText(hint.c_str(), hintSize);
		DrawText(hint.c_str(), (screenWidth_ - hintW) / 2, scoreY + 80, hintSize, LIGHTGRAY);
	}
}

// Draw intro screen
void GameManager::DrawIntro() 
{
	float alpha = 0.0f;
	float delayTime = 2.0f;
	float fadeTime = 2.0f;

	//delay before fade in
	if (introTimer_ < delayTime)
	{
		alpha = 0.0f;
	}

	// Fade in
	if (introTimer_ <= (delayTime + fadeTime))
	{
		alpha = (introTimer_ - delayTime)/ fadeTime ;
	}

	// Fade out
	else
	{
		alpha = (introDuration_ - introTimer_) / fadeTime;
	}

	// Clamp alpha between 0 and 1
	if (alpha < 0.0f) alpha = 0.0f;
	if (alpha > 1.0f) alpha = 1.0f;


	// Draw fading title
	Color fadeColor = Fade(RED, alpha);
	Color fadeColorAuth = Fade(RED, alpha);

	// Define Text and Sizes
	const char* title = "VECTOR CLASH";
	const char* author = "By Rillian";
	int titleSize = 120;
	int authorSize = 60;

	// Calculate Centered X Positions
	int titleX = (GetScreenWidth() - MeasureText(title, titleSize)) / 2;
	int authorX = (GetScreenWidth() - MeasureText(author, authorSize)) / 2;

	// Draw Text
	DrawText(title, titleX, GetScreenHeight() / 2 - 80, titleSize, fadeColor);
	DrawText(author, authorX, GetScreenHeight() / 2 + 20, authorSize, fadeColorAuth);
}

//SCORING SYSTEM
// save high scores to file
void GameManager::SaveHighScores()
{
	std::ofstream file(saveFileName_);
	if (file.is_open())
	{
		// Write stats line by line
		file << bestSurvivalTime_ << "\n";
		file << bestSoloTailLength_ << "\n";
		file << enemiesDupedBest_ << "\n";
		file.close();
	}
}

// Load high scores from file
void GameManager::LoadHighScores()
{
	std::ifstream file(saveFileName_);
	if (file.is_open())
	{
		// Read stats in the same order
		file >> bestSurvivalTime_;
		file >> bestSoloTailLength_;
		file >> enemiesDupedBest_;
		file.close();
	}
}

//ENDGAME BLOCK
// Check for game over conditions
void GameManager::CheckGameOver()
{
	int aliveCount = 0;
	ITailOwner* lastAlive = nullptr;

	for (ITailOwner* o : owners_)
	{
		if (o->IsAlive()) {
			aliveCount++;
			lastAlive = o;
		}
	}

	bool matchOver = false;
	if (initialPlayerCount_ > 1) {
		if (aliveCount <= 1) matchOver = true;
	}
	else {
		if (!playerOne_.IsAlive()) matchOver = true;
	}

	if (matchOver)
	{
		// Determine Winner
		if (aliveCount == 1 && lastAlive != nullptr && initialPlayerCount_ > 1)
		{
			// Find which player index matches the pointer
			if (lastAlive == &playerOne_) winner_ = 1;
			else if (lastAlive == &playerTwo_) winner_ = 2;
			else if (lastAlive == &playerThree_) winner_ = 3;
			else if (lastAlive == &playerFour_) winner_ = 4;
		}
		else
		{
			winner_ = 0;
		}

		// Save Stats
		if (winner_ > 0 && winner_ <= 4) playerWins_[winner_ - 1]++;
		if (selectedPlayers_ == 1) SaveHighScores();
		if (audioInitialized_) PlaySound(fxGameWinSound_);
		gameState_ = gameState::GameOver;
	}
}

// Destructor
GameManager::~GameManager()
{
	// Clean up dynamically allocated enemies
	for (Enemy* e : extraEnemies_) delete e;
	extraEnemies_.clear();

	if (audioInitialized_)
	{
		// Stop/unload music and close audio device if initialized
		for (auto& track : bgmTracks_) UnloadMusicStream(track);
		bgmTracks_.clear();
		UnloadSound(fxMenuDing_);
		UnloadSound(fxMenuSelect_);
		UnloadSound(fxCrash_);
		UnloadSound(fxPowerup_);
		UnloadSound(fxSpeedup_);
		UnloadSound(fxSpeeddown_);
		UnloadSound(fxGhost_);
		UnloadSound(fxSlice_);
		UnloadSound(fxIntroSound_);
		UnloadSound(fxGameWinSound_);


		CloseAudioDevice();
		audioInitialized_ = false;
	}
}