#include "GameManager.h"
#include <raymath.h>
#include <cmath>
#include <string>

// setting up the GameManager class

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
	, playerTwo_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, BLUE, KEY_A, KEY_D)
	, playerThree_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, GREEN, KEY_J, KEY_L)
	, playerFour_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, YELLOW, KEY_F, KEY_H)
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

	// ensure owners list points are valid (we keep object addresses stable)
	owners_.clear();
	if (playerCount >= 1) owners_.push_back(&playerOne_);
	if (playerCount >= 2) owners_.push_back(&playerTwo_);
	if (playerCount >= 3) owners_.push_back(&playerThree_);
	if (playerCount >= 4) owners_.push_back(&playerFour_);

	if (playerCount == 1) bestSoloTailLength_ = 0.0f;
	// remember how many players were active at start of the match
	initialPlayerCount_ = playerCount;
}

bool GameManager::CheckHeadAgainstAllTails(ITailOwner* subject)
{
	if (!subject->IsAlive()) return false;

	// mPlayer stores position as the head center (DrawRectanglePro was used with origin=center).
	// Treat GetPosition() as the head center (not top-left).
	Vector2 headCenter = subject->GetPosition();
	Vector2 headSize = subject->GetSize();
	float headRadius = std::max(headSize.x, headSize.y) * 0.5f;

	// bounds: head outside screen -> collision
	if (headCenter.x < 0.0f || headCenter.x > screenWidth_ || headCenter.y < 0.0f || headCenter.y > screenHeight_)
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
		Vector2 ownerCenter = owner->GetPosition();
		Vector2 ownerSize = owner->GetSize();

		// compute tail extent so broadphase includes long tails
		// this is the max distance from owner center to any tail segment
		// (used to avoid checking distant tails)
		float tailExtentSq = 0.0f;
		for (const auto& seg : tail)
		{
			// trail samples are stored as centers (mPlayer pushes m_position which is center)
			float d2 = Vector2DistanceSqr(ownerCenter, seg);
			if (d2 > tailExtentSq) tailExtentSq = d2;
		}
		float tailExtent = sqrtf(tailExtentSq);

		float ownerRadius = std::max(ownerSize.x, ownerSize.y) * 0.5f;
		float broadDist = headRadius + ownerRadius + tailExtent + 1.0f;
		if (Vector2DistanceSqr(headCenter, ownerCenter) > broadDist * broadDist) continue;

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
			Vector2 seg = tail[i];
			float dx = headCenter.x - seg.x;
			float dy = headCenter.y - seg.y;
			float distSq = dx * dx + dy * dy;
			float collisionDist = headRadius + std::max(ownerSize.x, ownerSize.y) * 0.5f;
			if (distSq < collisionDist * collisionDist)
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
					// find index in owners_ to set winner_ (owner ordering maps to player numbers)
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