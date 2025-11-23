#include "GameManager.h"
#include <raymath.h>
#include <cmath>
#include <string>
#include "Vector2D.h"


GameManager::GameManager(int screenWidth, int screenHeight)
	: screenWidth_(screenWidth)
	, screenHeight_(screenHeight)
	, halfScreenWidth_((float)screenWidth / 2.0f)
	, halfScreenHeight_((float)screenHeight / 2.0f)
	, spawnInvulnerableSeconds_(1.f) // increase this to avoid instant self-hits at spawn
	, spawnInvulnerableTimer_(0.0f)
	, state_(State::Menu)
	, selectedPlayers_(2) //default to 2 players
	, winner_(0)
	, bestSoloTailLength_(0.0f)
	// placeholder positions for reset
	, playerOne_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, RED)
	, playerTwo_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, ORANGE, KEY_A, KEY_D)
	, playerThree_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, GREEN, KEY_J, KEY_L)
	, playerFour_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, YELLOW, KEY_F, KEY_H)
	, enemyOne_({ 0, 0 }, { 10.0f, 10.0f }, 285.0f, BLUE)
{
	// initialize players and owners according to selectedPlayers_
	ResetPlayers(selectedPlayers_);
}

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
}

bool GameManager::CheckHeadAgainstAllTails(ITailOwner* subject)
{
	if (!subject->IsAlive()) return false;

	// mPlayer stores position as the head center (DrawRectanglePro was used with origin=center).
	// Treat GetPosition() as the head center (not top-left).
	Vector2 headCenterRay = subject->GetPosition();
	Vector2 headSize = subject->GetSize();
	Vector2D headVec = { headCenterRay.x, headCenterRay.y };
	float headRadius = std::max(headSize.x, headSize.y) * 0.5f;

	// bounds: head outside screen -> collision
	if (headVec.x < 0.0f || headVec.x > screenWidth_ || headVec.y < 0.0f || headVec.y > screenHeight_)
	{
		return true;
	}

	// check against all tail owners
	for (ITailOwner* owner : owners_)
	{
		if (!owner->IsAlive()) continue;

		//this specifically gets the tail segments from the owner
		// and stores it in a reference variable called tail
		// This is done to avoid copying the entire deque of tail segments,
		const auto& tail = owner->GetTailSegments();
		if (tail.empty()) continue;

		// Owner position is stored as center as well
		// Using Vector2D for distance calc
		Vector2 ownerCenterRay = owner->GetPosition();
		Vector2D ownerVec = { ownerCenterRay.x, ownerCenterRay.y };
		Vector2 ownerSize = owner->GetSize();

		// compute tail extent so broadphase includes long tails
		// this is the max distance from owner center to any tail segment
		// (used to avoid checking distant tails)
		float tailExtent = 0.0f;
		for (const auto& seg : tail)
		{
			Vector2D segVec = { seg.x, seg.y };
			// trail samples are stored as centers (mPlayer pushes m_position which is center)
			float d = ownerVec.DistanceToTarget(segVec);
			if (d > tailExtent) tailExtent = d;
		}

		float ownerRadius = std::max(ownerSize.x, ownerSize.y) * 0.5f;
		float broadDist = headRadius + ownerRadius + tailExtent + 1.0f;
		if (headVec.DistanceToTarget(ownerVec) > broadDist) continue;

		// self-collision: skip a small number of the most-recent samples that are
		// guaranteed to be very near the head (prevents instant spawn death).
		const float assumedSampleDistance = 3.0f; // matches mPlayer default
		size_t skipLast = 1;
		if (headRadius > 0.0f)
		{
			// minimal skip: cover the head radius in samples, add small margin (+2).
			skipLast = static_cast<size_t>(ceil(headRadius / assumedSampleDistance)) + 2;
		}

		size_t checkCount = tail.size();
		size_t endIndex = checkCount;
		size_t startIndex = 0;
		if (owner == subject && checkCount > skipLast) endIndex = checkCount - skipLast;
		else if (owner == subject && checkCount > 0) endIndex = 0; // skip all if tail smaller than skip

		for (size_t i = startIndex; i < endIndex; ++i)
		{
			// tail samples are centers
			// Using Vector2D for distance calc
			Vector2D segVec = { tail[i].x, tail[i].y };
			float actualDistance = headVec.DistanceToTarget(segVec);
			float collisionThreshold = headRadius + std::max(ownerSize.x, ownerSize.y) * 0.5f;

			if (actualDistance < collisionThreshold)
			{
				return true;
			}
		}
	}

	return false;
}

void GameManager::Update(float dt)
{
	// countdown spawn invul timer if active
	if (spawnInvulnerableTimer_ > 0.0f) spawnInvulnerableTimer_ -= dt;

	if (state_ == State::Menu)
	{
		// menu input: left/right to change player count, enter/space to start
		if (IsKeyPressed(KEY_LEFT))  selectedPlayers_ = std::max(1, selectedPlayers_ - 1);
		if (IsKeyPressed(KEY_RIGHT)) selectedPlayers_ = std::min(4, selectedPlayers_ + 1);

		if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
		{
			ResetPlayers(selectedPlayers_);
			spawnInvulnerableTimer_ = spawnInvulnerableSeconds_;
			state_ = State::Running;
		}

		return;
	}
	if (state_ == State::Running)
	{
		if (selectedPlayers_ == 1)
		{
			// 1. RESPAWN LOGIC
			if (!enemyOne_.IsAlive())
			{
				enemyRespawnTimer_ -= dt;
				if (enemyRespawnTimer_ <= 0.0f)
				{
					// Pick a spot randomly not to close to player
					Vector2 spawnPos;
					do {
						spawnPos = { (float)GetRandomValue(50, screenWidth_ - 50),
									 (float)GetRandomValue(50, screenHeight_ - 50) };
					} while (Vector2Distance(spawnPos, playerOne_.GetPosition()) < 400.0f); //distance in pixels

					enemyOne_.Reset(spawnPos);
					enemyRespawnTimer_ = enemyRespawnDelay_;
				}
			}
			// 2. MOVEMENT PREDICTION LOGIC
			else if (playerOne_.IsAlive())
			{
				// Convert positions to Vector2D for math
				Vector2D currPosVec = { playerOne_.GetPosition().x, playerOne_.GetPosition().y };
				Vector2D prevPosVec = { previousPlayerPos_.x, previousPlayerPos_.y };

				Vector2D velocity = prevPosVec.VectorTowardsTarget(currPosVec);

				// Adjust this to aim further ahead or closer
				// We use ScaleVector to multiply the movement vector
				Vector2D aimOffset = velocity.ScaleVector(45.0f);

				// Calculate the predicted point
				Vector2D predictedPoint = currPosVec.SetVectorOffset(aimOffset);

				// Apply target
				// We convert back to Raylib Vector2 just to pass it to SetTarget
				enemyOne_.SetTarget({ predictedPoint.x, predictedPoint.y });
			}
			else
			{
				// When the Player is dead, enemy returns to center
				enemyOne_.SetTarget({ halfScreenWidth_, halfScreenHeight_ });
			}

			// Update previous position for the next frame's calculation
			previousPlayerPos_ = playerOne_.GetPosition();

		}
		// update only active owners
		for (ITailOwner* o : owners_) o->OwnerUpdate(dt);

		if (selectedPlayers_ == 1 && !owners_.empty())
		{
			float cur = owners_[0]->GetTailPixelLength();
			if (cur > bestSoloTailLength_) bestSoloTailLength_ = cur;
		}

		// if invul expired perform collision checks
		if (spawnInvulnerableTimer_ <= 0.0f)
		{
			for (ITailOwner* o : owners_)
			{
				if (o->IsAlive() && CheckHeadAgainstAllTails(o))
				{
					o->OwnerKill();
				}
			}
		}

		

		// determine game over by counting alive owners
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

			// For multiplayer matches (initialPlayerCount_ > 1) game-over when <= 1 remains.
			// For solo mode (initialPlayerCount_ == 1) treat game-over only when player is dead (aliveCount == 0).
			bool matchOver = false;
			if (initialPlayerCount_ > 1)
			{
				if (aliveCount <= 1) matchOver = true;
			}
			else // solo
			{
				if (aliveCount == 0) matchOver = true;
			}

			if (matchOver)
			{
				state_ = State::GameOver;
				if (aliveCount == 1 && lastAlive != nullptr)
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
					// no one alive -> draw (or solo player died)
					winner_ = 0;
				}
			}
		}
	}
	else // GameOver: Space = restart, Enter = back to menu
	{
		if (IsKeyPressed(KEY_SPACE))
		{
			ResetPlayers(selectedPlayers_);
			state_ = State::Running;
			winner_ = 0;
		}
		else if (IsKeyPressed(KEY_ENTER))
		{
			// return to menu so user can change selectedPlayers_
			state_ = State::Menu;
		}
	}
}

void GameManager::Draw()
{
	// Draw menu
	if (state_ == State::Menu)
	{
		std::string title = "VECTOR CLASH";
		int fs = 80;
		int tw = MeasureText(title.c_str(), fs);
		DrawText(title.c_str(), (screenWidth_ - tw) / 2, 120, fs, RAYWHITE);

		std::string instr = "Use LEFT / RIGHT to pick players, ENTER or SPACE to start";
		int isz = 20;
		int iw = MeasureText(instr.c_str(), isz);
		DrawText(instr.c_str(), (screenWidth_ - iw) / 2, 220, isz, LIGHTGRAY);

		// draw selection
		std::string sel = "Players: " + std::to_string(selectedPlayers_);
		int sfs = 48;
		int sw = MeasureText(sel.c_str(), sfs);
		DrawText(sel.c_str(), (screenWidth_ - sw) / 2, 320, sfs, YELLOW);

		return;
	}

	// draw only active owners
	//more specifically, this loop iterates through each
	// ITailOwner pointer in the owners_ vector and calls the OwnerDraw() method on each one.
	for (ITailOwner* o : owners_) o->OwnerDraw();

	// debug overlay, top left
	{
		std::string dbg = "P1 alive: " + std::string(playerOne_.IsAlive() ? "YES" : "NO")
			+ "  tailPts: " + std::to_string(playerOne_.GetTailSegments().size())
			+ "  pos: (" + std::to_string((int)playerOne_.GetPosition().x) + "," + std::to_string((int)playerOne_.GetPosition().y) + ")";
		DrawText(dbg.c_str(), 10, 10, 16, WHITE);

		std::string dbg2 = "P2 alive: " + std::string(playerTwo_.IsAlive() ? "YES" : "NO")
			+ "  tailPts: " + std::to_string(playerTwo_.GetTailSegments().size())
			+ "  pos: (" + std::to_string((int)playerTwo_.GetPosition().x) + "," + std::to_string((int)playerTwo_.GetPosition().y) + ")";
		DrawText(dbg2.c_str(), 10, 30, 16, WHITE);

		std::string timer = "Spawn safe: " + std::to_string((double)spawnInvulnerableTimer_);
		DrawText(timer.c_str(), 10, 50, 16, YELLOW);
	}

	// GameOver overlay winner/looser message
	if (state_ == State::GameOver)
	{
		if (selectedPlayers_ == 1)
		{
			float finalLen = owners_.empty() ? 0.0f : owners_[0]->GetTailPixelLength();
			std::string msg = "Solo run finished";
			int fontSize = 40;
			int textWidth = MeasureText(msg.c_str(), fontSize);
			DrawText(msg.c_str(), (screenWidth_ - textWidth) / 2, screenHeight_ / 2 - 80, fontSize, RAYWHITE);

			char buf[128];
			snprintf(buf, sizeof(buf), "Your tail: %.0f px", finalLen);
			DrawText(buf, (screenWidth_ - MeasureText(buf, 30)) / 2, screenHeight_ / 2 - 20, 30, RAYWHITE);

			snprintf(buf, sizeof(buf), "Best: %.0f px", bestSoloTailLength_);
			DrawText(buf, (screenWidth_ - MeasureText(buf, 24)) / 2, screenHeight_ / 2 + 20, 24, LIGHTGRAY);

			std::string hint = "Space = Retry, Enter = Menu";
			int hintSize = 20;
			int hintW = MeasureText(hint.c_str(), hintSize);
			DrawText(hint.c_str(), (screenWidth_ - hintW) / 2, screenHeight_ / 2 + 60, hintSize, LIGHTGRAY);
		}
		else
		{
			std::string msg;
			if (winner_ == 1) msg = "Player ONE WINS! Move to restart";
			else if (winner_ == 2) msg = "Player TWO WINS! Move to restart";
			else if (winner_ == 3) msg = "Player THREE WINS! Move to restart";
			else if (winner_ == 4) msg = "Player FOUR WINS! Move to restart";
			else msg = "DRAW! Move to restart";

			int fontSize = 50;
			int textWidth = MeasureText(msg.c_str(), fontSize);
			DrawText(msg.c_str(), (screenWidth_ - textWidth) / 2, screenHeight_ / 2 - 20, fontSize, RAYWHITE);

			std::string hint = "Press Space to restart, Enter to return to Menu";
			int hintSize = 28;
			int hintW = MeasureText(hint.c_str(), hintSize);
			DrawText(hint.c_str(), (screenWidth_ - hintW) / 2, screenHeight_ / 2 + 30, hintSize, LIGHTGRAY);
		}
	}
}