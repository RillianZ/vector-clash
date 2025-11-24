#include "GameManager.h"
#include <raymath.h>
#include <cmath>
#include <string>
#include "Vector2D.h"

// Main Constructor
GameManager::GameManager(int screenWidth, int screenHeight)
	: screenWidth_(screenWidth)
	, screenHeight_(screenHeight)
	, halfScreenWidth_((float)screenWidth / 2.0f)
	, halfScreenHeight_((float)screenHeight / 2.0f)
	, spawnInvulnerableSeconds_(1.f) // increase this to avoid instant self-hits at spawn
	, spawnInvulnerableTimer_(0.0f) 
	, state_(State::Menu)
	, selectedPlayers_(1) //default to 1 for quicker testing
	, winner_(0)
	, bestSoloTailLength_(0.0f)
	, enemiesDupedCurrent_(0)  
	, enemiesDupedBest_(0)     
	, hardMode_(false)         
	, hardModeTimer_(0.0f)     
	, playerOne_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, RED)
	, playerTwo_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, ORANGE, KEY_A, KEY_D)
	, playerThree_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, GREEN, KEY_J, KEY_L)
	, playerFour_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, YELLOW, KEY_F, KEY_H)
	, enemyOne_({ 0, 0 }, { 16.0f, 16.0f }, 255.0f, 200.0f, BLUE, screenWidth, screenHeight)
	, hasBgm_(false)
	, audioInitialized_(false)
	, selectedMapType_(MapType::Clean)
	, powerupSpawnTimer_(0.0f)

		{
			// intialize player wins vector
			playerWins_ = { 0, 0, 0, 0 };
			// initialize players and owners according to selectedPlayers_
			ResetPlayers(selectedPlayers_);
			// Initialize audio device and load background music
			InitAudioDevice();
			audioInitialized_ = IsAudioDeviceReady();
			if (audioInitialized_)
			{
				//bgm loads and plays at half volume, sets bool to true to avoid reloading
				bgm_ = LoadMusicStream("resources/background.mp3");
				SetMusicVolume(bgm_, 0.5f);
				PlayMusicStream(bgm_);
				hasBgm_ = true;

				// Load sound effects
				fxMenuDing_ = LoadSound("resources/menu_ding.mp3");
				fxMenuSelect_ = LoadSound("resources/menu_select.mp3");
				fxCrash_ = LoadSound("resources/crash.mp3");
				fxPowerup_ = LoadSound("resources/powerup.mp3");
			}
		}

//Player reset function (maybe merge this with destructor somehow?)
void GameManager::ResetPlayers(int playerCount)
{
	const float sep = 300.0f;
	// reset player positions, keys and facing angle
	playerOne_.Reset({ halfScreenWidth_ - sep, halfScreenHeight_ }, KEY_LEFT, KEY_RIGHT, 0.0f);
	playerTwo_.Reset({ halfScreenWidth_ + sep, halfScreenHeight_ }, KEY_A, KEY_D, 180.0f);
	playerThree_.Reset({ halfScreenWidth_, halfScreenHeight_ - sep }, KEY_J, KEY_L, 90.0f);
	playerFour_.Reset({ halfScreenWidth_, halfScreenHeight_ + sep }, KEY_F, KEY_H, 270.0f);

	// reset invulnerability timer and winner
	spawnInvulnerableTimer_ = spawnInvulnerableSeconds_;
	winner_ = 0;

	// Reset Solo Stats
	enemiesDupedCurrent_ = 0;
	hardModeTimer_ = hardModeInterval_; // Reset spawn timer

	// Clean up old extra enemies
	for (Enemy* e : extraEnemies_) delete e;
	extraEnemies_.clear();

	// Resets Respawn timer
	enemyRespawnTimer_ = enemyRespawnDelay_;

	// Resets Tracking
	previousPlayerPos_ = playerOne_.GetPosition();

	// ensure owners list points are valid (we keep object addresses stable)
	owners_.clear();

	// ALWAYS add Player 1
	if (playerCount >= 1) owners_.push_back(&playerOne_);

	// If singeplayer add enemy
	if (playerCount == 1)
	{
		// Spawn enemy on the opposite side of the screen from Player 1
		enemyOne_.Reset({ halfScreenWidth_ + sep, halfScreenHeight_ });
		//Set player reference for enemy AI
		enemyOne_.SetPlayerReference(&playerOne_);

		//Set obstacles reference for enemy collision detection
		enemyOne_.SetObstaclesReference(&mapGen_.GetObstacles());

		owners_.push_back(&enemyOne_);
	}
	// IF MULTIPLAYER: Add other human players
	else
	{
		if (playerCount >= 2) owners_.push_back(&playerTwo_);
		if (playerCount >= 3) owners_.push_back(&playerThree_);
		if (playerCount >= 4) owners_.push_back(&playerFour_);
	}

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

// Generic tail collision detection function
// Returns pointer to victim ITailOwner if collision detected, else nullptr
ITailOwner* GameManager::GetTailCollisionVictim(ITailOwner* subject)
{
	if (!subject->IsAlive()) return nullptr;

	Vector2 headCenterRay = subject->GetPosition();
	Vector2 headSize = subject->GetSize();
	Vector2D headVec = { headCenterRay.x, headCenterRay.y };
	float headRadius = std::max(headSize.x, headSize.y) * 0.5f;

	// Check against all tail owners
	for (ITailOwner* owner : owners_)
	{
		if (!owner->IsAlive()) continue;
		const auto& tail = owner->GetTailSegments();
		if (tail.empty()) continue;

		// Skip self-collision checks for recent points
		size_t skipLast = (owner == subject) ? 20 : 0;
		size_t limit = tail.size();
		if (limit > skipLast) limit -= skipLast;
		else limit = 0;

		for (size_t i = 0; i < limit; i += 2) // Optimization: check every 2nd point
		{
			Vector2D segVec = { tail[i].x, tail[i].y };
			if (headVec.DistanceToTarget(segVec) < (headRadius + 5.0f))
			{
				return owner;
			}
		}
	}
	return nullptr;
}

// Main per-frame update function
//Most of the game logic is here!
void GameManager::Update(float dt)
{
	// countdown spawn invul timer if active
	if (spawnInvulnerableTimer_ > 0.0f) spawnInvulnerableTimer_ -= dt;

	// Menu state logic
	if (state_ == State::Menu)
	{
		// Track if we moved the cursor
		bool changed = false; 

		//any change makes a sound
		// menu input: left/right to change player count, enter/space to start, up/down to change map type, and H for hard mode
		if (IsKeyPressed(KEY_LEFT)) { selectedPlayers_ = std::max(1, selectedPlayers_ - 1); changed = true; }
		if (IsKeyPressed(KEY_RIGHT)) { selectedPlayers_ = std::min(4, selectedPlayers_ + 1); changed = true; }
		if (IsKeyPressed(KEY_UP))
		{
			int type = (int)selectedMapType_;
			if (type > 0) { selectedMapType_ = (MapType)(type - 1); changed = true; }
		}
		if (IsKeyPressed(KEY_DOWN))
		{
			int type = (int)selectedMapType_;
			if (type < 2) { selectedMapType_ = (MapType)(type + 1); changed = true; }
		}
		if (IsKeyPressed(KEY_H)) { hardMode_ = !hardMode_; changed = true; }

		// Play Ding on change
		if (changed && audioInitialized_) PlaySound(fxMenuDing_);

		if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
		{
			//Play Swoosh on start
			if (audioInitialized_) PlaySound(fxMenuSelect_);
			ResetPlayers(selectedPlayers_);
			mapGen_.GenerateMap(selectedMapType_, screenWidth_, screenHeight_);
			spawnInvulnerableTimer_ = spawnInvulnerableSeconds_;
			state_ = State::Running;
		}
		// Update audio stream while in menu
		if (audioInitialized_ && hasBgm_)
		{
			UpdateMusicStream(bgm_);
			if (!IsMusicStreamPlaying(bgm_)) PlayMusicStream(bgm_);
		}

		return;
	}
	//Main running state game logic!
	if (state_ == State::Running)
	{
		// POWERUP LOGIC
		powerupSpawnTimer_ -= dt;
		if (powerupSpawnTimer_ <= 0.0f)
		{
			SpawnPowerUp();
			if (hardMode_) SpawnPowerUp();
			powerupSpawnTimer_ = (float)GetRandomValue(5, 10);
		}

		for (auto& p : powerups_) p.Update(dt);

		// Check Powerup Collection
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
						// Play powerup sound
						if (audioInitialized_) PlaySound(fxPowerup_);
					}
				}
			}
		}

		// Clean inactive powerups
		powerups_.erase(std::remove_if(powerups_.begin(), powerups_.end(),
			[](const PowerUp& p) { return !p.IsActive(); }), powerups_.end());
		//Solo mode enemy logic
		if (selectedPlayers_ == 1)
		{
			// RESPAWN LOGIC
			if (!enemyOne_.IsAlive())
			{
				enemyRespawnTimer_ -= dt;
				if (enemyRespawnTimer_ <= 0.0f)
				{
					// Pick a spot randomly not to close to player
					// and within safe X bounds (avoids spawning too close to edges)
					int safeMinX = (int)(screenWidth_ * 0.20f);
					int safeMaxX = (int)(screenWidth_ * 0.80f);
					Vector2 spawnPos;
					do 
					{
						spawnPos = {(float)GetRandomValue(safeMinX, safeMaxX), (float)GetRandomValue(50, screenHeight_ - 50)};
					} 
					while (Vector2Distance(spawnPos, playerOne_.GetPosition()) < 400.0f); //distance in pixels

					enemyOne_.Reset(spawnPos);
					enemyRespawnTimer_ = enemyRespawnDelay_;
				}
			}
			//  MOVEMENT PREDICTION LOGIC
			else if (playerOne_.IsAlive())
			{
				// Convert positions to Vector2D for math
				Vector2D currPosVec = { playerOne_.GetPosition().x, playerOne_.GetPosition().y };
				Vector2D prevPosVec = { previousPlayerPos_.x, previousPlayerPos_.y };

				Vector2D velocity = prevPosVec.VectorTowardsTarget(currPosVec);

				// Adjust this to aim further ahead or closer for enemy
				// We use ScaleVector to multiply the movement vector
				// basically knowing the players direction and speed and multiplying it and aiming that direction
				Vector2D aimOffset = velocity.ScaleVector(35.0f);

				// Calculate the predicted point
				Vector2D predictedPoint = currPosVec.SetVectorOffset(aimOffset);

				// Apply target
				enemyOne_.SetTarget({ predictedPoint.x, predictedPoint.y });
			}
			else
			{
				// When the Player is dead, enemy returns to center
				enemyOne_.SetTarget({ halfScreenWidth_, halfScreenHeight_ });
			}

			//  Hard Mode Logic 
			if (hardMode_ && playerOne_.IsAlive())
			{
				// Spawn Timer
				hardModeTimer_ -= dt;
				if (hardModeTimer_ <= 0.0f)
				{
					hardModeTimer_ = hardModeInterval_;

					// Spawn New Enemy
					int safeMinX = (int)(screenWidth_ * 0.20f);
					int safeMaxX = (int)(screenWidth_ * 0.80f);
					Vector2 spawnPos = { (float)GetRandomValue(safeMinX, safeMaxX),
										 (float)GetRandomValue(50, screenHeight_ - 50) };

					// Create new Enemy with random color and add to extraEnemies_ vector
					Color randomColor = ColorFromHSV((float)GetRandomValue(0, 360), 0.8f, 0.9f);
					Enemy* newEnemy = new Enemy(spawnPos, { 16.0f, 16.0f }, 260.0f, 210.0f, randomColor, screenWidth_, screenHeight_);
					newEnemy->SetPlayerReference(&playerOne_);

					//Set obstacles reference for enemy collision detection
					newEnemy->SetObstaclesReference(&mapGen_.GetObstacles());

					// Add to extra enemies list and owners list
					extraEnemies_.push_back(newEnemy);
					owners_.push_back(newEnemy);
				}

				// Update extra enemies targets according to player position so the enemies know where to go
				Vector2D currPosVec = { playerOne_.GetPosition().x, playerOne_.GetPosition().y };
				for (Enemy* e : extraEnemies_)
				{
					if (e->IsAlive()) e->SetTarget({ currPosVec.x, currPosVec.y }); // Simple direct tracking
				}
			}

			// Clean up dead Hard Mode enemies 
			// Going backwards through the vector to safely remove elements
			for (int i = extraEnemies_.size() - 1; i >= 0; i--)
			{
				if (!extraEnemies_[i]->IsAlive())
				{
					// Remove from owners_ list
					ITailOwner* deadPtr = extraEnemies_[i];
					owners_.erase(std::remove(owners_.begin(), owners_.end(), deadPtr), owners_.end());

					// Delete memory
					delete extraEnemies_[i];

					// Remove from vector
					extraEnemies_.erase(extraEnemies_.begin() + i);
				}
			}

			// Update previous position for the next frame's calculation
			previousPlayerPos_ = playerOne_.GetPosition();
		}
		// update only active owners
		for (ITailOwner* o : owners_) o->OwnerUpdate(dt);

		if (selectedPlayers_ == 1 && !owners_.empty())
		{
			// Update best solo tail length
			float cur = owners_[0]->GetTailPixelLength();
			if (cur > bestSoloTailLength_) bestSoloTailLength_ = cur;
			// Update best enemies duped count
			if (enemiesDupedCurrent_ > enemiesDupedBest_) enemiesDupedBest_ = enemiesDupedCurrent_;
		}

		// Main collision checks (skip if in spawn invulnerability)
		if (spawnInvulnerableTimer_ <= 0.0f)
		{
			std::vector<ITailOwner*> pendingDeaths;

			for (ITailOwner* o : owners_)
			{
				// Skip dead owners
				if (!o->IsAlive()) continue;
				// Check if owner is a player and if they are in ghost mode
				mPlayer* player = dynamic_cast<mPlayer*>(o);
				bool isGhost = (player && player->IsPhasing());
				bool markedForDeath = false;

				// Wall Checks
				if (CheckHeadAgainstMap(o))
				{
					markedForDeath = true;
				}

				// 2. Tail Checks
				// Get collision victim if any
				ITailOwner* victim = GetTailCollisionVictim(o);
				if (victim && !markedForDeath)
				{
					if (isGhost)
					{
						// Phase Shift: Do nothing, pass through
					}
					else if (player && player->HasTailCutter())
					{
						// CUTTER LOGIC
						player->ConsumeTailCutter();

						// Clear the victim's tail (entire tail for simplicity)
						if (mPlayer* vp = dynamic_cast<mPlayer*>(victim)) vp->ClearTail();
						else if (Enemy* ve = dynamic_cast<Enemy*>(victim)) ve->ClearTail();
					}
					else
					{
						markedForDeath = true;
					}
				}
				if (markedForDeath)
				{
					pendingDeaths.push_back(o);
				}
			}
			// Execute all deaths simultaneously
			bool someoneDied = false;
			for (ITailOwner* victim : pendingDeaths)
			{
				// Check IsAlive() again to prevent double-counting 
				if (victim->IsAlive())
				{
					victim->OwnerKill();
					if (selectedPlayers_ == 1 && victim != &playerOne_) enemiesDupedCurrent_++;
					someoneDied = true;
				}
			}
			// Play crash sound if anyone died
			if (someoneDied && audioInitialized_) PlaySound(fxCrash_);
		}

		//  GAME OVER check by counting alive owners
		{
			int aliveCount = 0;
			ITailOwner* lastAlive = nullptr;
			for (ITailOwner* o : owners_)
			{
				if (o->IsAlive())
				{
					++aliveCount;
					lastAlive = o;
				}
			}

			// For multiplayer matches  game-over when one left
			// For solo mode treat game-over only when player is dead
			bool matchOver = false;
			if (initialPlayerCount_ > 1)
			{
				if (aliveCount <= 1) matchOver = true;
			}
			else // solo game
				if (!playerOne_.IsAlive())
				{
					matchOver = true;
				}

			if (matchOver)
			{
				
				if (aliveCount == 1 && lastAlive != nullptr && initialPlayerCount_ > 1)
				{
					// checks index for winner
					int idx = -1;
					for (size_t i = 0; i < owners_.size(); ++i)
					{
						if (owners_[i] == lastAlive) { idx = (int)i; break; }
					}
					winner_ = (idx >= 0) ? (idx + 1) : 0;
				}
				else
				{
					// no one alive sets winner to 0; to catch weird states
					winner_ = 0;
				}

				// Increment Score for winner if multiplayer
				if (winner_ > 0 && winner_ <= 4)
				{
					playerWins_[winner_ - 1]++;
				}

				//sets the game state to game over
				state_ = State::GameOver;
			}
		}

		// Update music stream each frame while running
		if (audioInitialized_ && hasBgm_)
		{
			UpdateMusicStream(bgm_);
			if (!IsMusicStreamPlaying(bgm_)) PlayMusicStream(bgm_);
			SetMusicVolume(bgm_, 1.0f); // Set volume up to 100 during gameplay
		}
	}
	// else GameOver state: Space = restart players and also calls new mapgen, Enter = back to menu 
	else 
	{
		if (IsKeyPressed(KEY_SPACE))
		{
			ResetPlayers(selectedPlayers_);
			mapGen_.GenerateMap(selectedMapType_, screenWidth_, screenHeight_);
			state_ = State::Running;
			winner_ = 0;
		}
		else if (IsKeyPressed(KEY_ENTER))
		{
			// return to menu so user can change selectedPlayers_
			state_ = State::Menu;
		}
		// Keep updating music while on GameOver screen
		if (audioInitialized_ && hasBgm_)
		{
			UpdateMusicStream(bgm_);
			if (!IsMusicStreamPlaying(bgm_)) PlayMusicStream(bgm_);
			SetMusicVolume(bgm_, 0.5f); // Set volume to 50%
		}
	}
}

// Draw current game state (call between BeginDrawing/EndDrawing)
void GameManager::Draw()
{
	// Draw menu
	if (state_ == State::Menu)
	{
		// Draw Dynamic Controls info
		int startY = 600;
		DrawText("Controls:", (screenWidth_ - MeasureText("Controls:", 20)) / 2, startY, 20, LIGHTGRAY);

		int boxWidth = 250;
		int totalWidth = selectedPlayers_ * boxWidth;
		int startX = (screenWidth_ - totalWidth) / 2;

		for (int i = 0; i < selectedPlayers_; i++)
		{
			int x = startX + (i * boxWidth);
			Color pColor = WHITE;
			std::string label = "P" + std::to_string(i + 1);
			std::string keys = "";

			switch (i)
			{
			case 0: pColor = RED;    keys = "LEFT / RIGHT"; break;
			case 1: pColor = ORANGE; keys = "A / D"; break;
			case 2: pColor = GREEN;  keys = "J / L"; break;
			case 3: pColor = YELLOW; keys = "F / H"; break;
			}

			DrawText(label.c_str(), x + 100, startY + 40, 30, pColor);
			DrawText(keys.c_str(), x + 60, startY + 80, 20, RAYWHITE);
		}

		// Hard mode indicator
		if (hardMode_ == true)
		{
			DrawText("HARD MODE ENABLED", screenWidth_ / 2 - 150, 50, 30, BLACK);
			ClearBackground(RED);
		}

		std::string title = "VECTOR CLASH";
		int fs = 80;
		int tw = MeasureText(title.c_str(), fs);
		DrawText(title.c_str(), (screenWidth_ - tw) / 2, 120, fs, RAYWHITE);

		std::string instr = "'Arrows' to Select players and maptype, 'H' Toggles Hard mode, 'ENTER' or 'SPACE' to start";
		int isz = 20;
		int iw = MeasureText(instr.c_str(), isz);
		DrawText(instr.c_str(), (screenWidth_ - iw) / 2, 220, isz, LIGHTGRAY);

		// draw selection of players
		std::string sel = "Players: " + std::to_string(selectedPlayers_);
		int sfs = 48;
		int sw = MeasureText(sel.c_str(), sfs);
		DrawText(sel.c_str(), (screenWidth_ - sw) / 2, 320, sfs, YELLOW);

		// draw selection of map type
		std::string mapName;
		std::string mapDesc;
		switch (selectedMapType_)
		{
		case MapType::Clean:
			mapName = "Map: CLEAN";
			mapDesc = "No obstacles. Pure skill.";
			break;
		case MapType::Fixed:
			mapName = "Map: FIXED";
			mapDesc = "Static central obstacles.";
			break;
		case MapType::Wave:
			mapName = "Map: WAVE";
			mapDesc = "Procedural generated waves.";
			break;
		}

		//draws text for map name
		int mfs = 40;
		int mw = MeasureText(mapName.c_str(), mfs);
		DrawText(mapName.c_str(), (screenWidth_ - mw) / 2, 420, mfs, GREEN);
		//draws text for map description
		int dfs = 20;
		int dw = MeasureText(mapDesc.c_str(), dfs);
		DrawText(mapDesc.c_str(), (screenWidth_ - dw) / 2, 460, dfs, LIGHTGRAY);

		return;
	}

	// Draw map 
	mapGen_.Draw();

	// Draw power-ups by iterating through the powerups_ vector and calling Draw on each powerup
	for (auto& p : powerups_) p.Draw();

	//this loop iterates through each ITailOwner pointer in the owners_ vector and calls draw on each
	for (ITailOwner* o : owners_) o->OwnerDraw();

	// debug overlay/HUD, top left
	{
		//Show duped count in HUD for Solo
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

			if (hardMode_)
			{
				std::string spawnT = "Hard Mode Reinforcements: " + std::to_string((int)hardModeTimer_);
				DrawText(spawnT.c_str(), screenWidth_ / 2 - 220, 10, 30, RED);
			}
		}
		
		/*
		* Commented out debug info for players
		std::string dbg2 = "P2 alive: " + std::string(playerTwo_.IsAlive() ? "YES" : "NO")
			+ "  tailPts: " + std::to_string(playerTwo_.GetTailSegments().size())
			+ "  pos: (" + std::to_string((int)playerTwo_.GetPosition().x) + "," + std::to_string((int)playerTwo_.GetPosition().y) + ")";
		DrawText(dbg2.c_str(), 10, 30, 16, WHITE);

		std::string timer = "Spawn safe: " + std::to_string((double)spawnInvulnerableTimer_);
		DrawText(timer.c_str(), 10, 50, 16, YELLOW);
		*/
	}

	// GameOver overlay winner/looser message
	if (state_ == State::GameOver)
	{
		// draw different messages for solo vs multiplayer
		//uses measuretext to center the text on screen
		//usess snprintf to format the tail length values into strings for display
		//Might clean this up later
		if (selectedPlayers_ == 1)
		{
			float finalLen = owners_.empty() ? 0.0f : owners_[0]->GetTailPixelLength();
			std::string msg = "Solo run finished";
			int fontSize = 40;
			int textWidth = MeasureText(msg.c_str(), fontSize);
			DrawText(msg.c_str(), (screenWidth_ - textWidth) / 2, screenHeight_ / 2 - 80, fontSize, RAYWHITE);

			char buf[128];
			snprintf(buf, sizeof(buf), "Length: %.0f px (Best: %.0f)", finalLen, bestSoloTailLength_);
			DrawText(buf, (screenWidth_ - MeasureText(buf, 30)) / 2, screenHeight_ / 2 - 40, 30, RAYWHITE);

			snprintf(buf, sizeof(buf), "Enemies Duped: %d (Best: %d)", enemiesDupedCurrent_, enemiesDupedBest_);
			DrawText(buf, (screenWidth_ - MeasureText(buf, 30)) / 2, screenHeight_ / 2, 30, ORANGE);

			std::string hint = "Space = Retry, Enter = Menu";
			int hintSize = 20;
			int hintW = MeasureText(hint.c_str(), hintSize);
			DrawText(hint.c_str(), (screenWidth_ - hintW) / 2, screenHeight_ / 2 + 60, hintSize, LIGHTGRAY);
		}
		else
		{
			std::string msg;
			if (winner_ == 1) msg = "Player ONE WINS! Space to restart, Enter for Main menu";
			else if (winner_ == 2) msg = "Player TWO WINS! Space to restart, Enter for Main menu";
			else if (winner_ == 3) msg = "Player THREE WINS! Space to restart, Enter for Main menu";
			else if (winner_ == 4) msg = "Player FOUR WINS! Space to restart, Enter for Main menu";
			else msg = "DRAW! Space to restart, Enter for Main menu";

			int fontSize = 50;
			int textWidth = MeasureText(msg.c_str(), fontSize);
			DrawText(msg.c_str(), (screenWidth_ - textWidth) / 2, screenHeight_ / 2 - 20, fontSize, RAYWHITE);

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
				DrawText(s.c_str(), scoreX + (i * scoreGap), scoreY, 30, pColor);
			}

			std::string hint = "Press Space to restart, Enter to return to Menu";
			int hintSize = 28;
			int hintW = MeasureText(hint.c_str(), hintSize);
			DrawText(hint.c_str(), (screenWidth_ - hintW) / 2, scoreY + 80, hintSize, LIGHTGRAY);
		}
	}
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

// Destructor
GameManager::~GameManager()
{
	// Clean up dynamically allocated enemies
	for (Enemy* e : extraEnemies_) delete e;
	extraEnemies_.clear();

	

	if (audioInitialized_)
	{
		// Stop/unload music and close audio device if initialized
		if (hasBgm_)
		{
			StopMusicStream(bgm_);
			UnloadMusicStream(bgm_);
		}
		UnloadSound(fxMenuDing_);
		UnloadSound(fxMenuSelect_);
		UnloadSound(fxCrash_);
		UnloadSound(fxPowerup_);

		CloseAudioDevice();
		audioInitialized_ = false;
	}
}