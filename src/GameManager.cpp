#include "GameManager.h"
#include <raymath.h>
#include <cmath>
#include <string>

// Helper local lambda copied logic removed from main previously

GameManager::GameManager(int screenWidth, int screenHeight)
    : screenWidth_(screenWidth)
    , screenHeight_(screenHeight)
    , halfScreenWidth_((float)screenWidth / 2.0f)
    , halfScreenHeight_((float)screenHeight / 2.0f)
    , spawnInvulnerableSeconds_(0.5f)
    , spawnInvulnerableTimer_(0.0f)
    , state_(State::Running)
    , winner_(0)
    // construct players with placeholder positions; ResetPlayers will set proper start state
    , playerOne_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, RED)
    , playerTwo_({ halfScreenWidth_, halfScreenHeight_ }, { 16.0f, 16.0f }, 200.0f, 180.0f, BLUE, KEY_A, KEY_D)
{
    // initialize players and owners
    ResetPlayers();
    owners_.clear();
    owners_.push_back(&playerOne_);
    owners_.push_back(&playerTwo_);
}

void GameManager::ResetPlayers()
{
    const float sep = 300.0f;
    // reset player positions, keys and facing angle
    playerOne_.Reset({ halfScreenWidth_ - sep, halfScreenHeight_ }, KEY_LEFT, KEY_RIGHT, 0.0f);
    playerTwo_.Reset({ halfScreenWidth_ + sep, halfScreenHeight_ }, KEY_A, KEY_D, 180.0f);

    // reset state & invulnerability timer
    spawnInvulnerableTimer_ = spawnInvulnerableSeconds_;
    state_ = State::Running;
    winner_ = 0;

    // ensure owners list points are valid (we keep object addresses stable)
    owners_.clear();
    owners_.push_back(&playerOne_);
    owners_.push_back(&playerTwo_);
}

bool GameManager::CheckHeadAgainstAllTails(ITailOwner* subject)
{
    if (!subject->IsAlive()) return false;

    Vector2 headPos = subject->GetPosition();
    Vector2 headSize = subject->GetSize();
    Vector2 headCenter = { headPos.x + headSize.x * 0.5f, headPos.y + headSize.y * 0.5f };
    float headRadius = std::max(headSize.x, headSize.y) * 0.5f;

    // bounds: head outside screen -> collision
    if (headCenter.x < 0.0f || headCenter.x > screenWidth_ || headCenter.y < 0.0f || headCenter.y > screenHeight_)
    {
        return true;
    }

    for (ITailOwner* owner : owners_)
    {
        if (!owner->IsAlive()) continue;

        const auto& tail = owner->GetTailSegments();
        if (tail.empty()) continue;

        Vector2 ownerPos = owner->GetPosition();
        Vector2 ownerSize = owner->GetSize();
        Vector2 ownerCenter = { ownerPos.x + ownerSize.x * 0.5f, ownerPos.y + ownerSize.y * 0.5f };

        // compute tail extent so broadphase includes long tails
        float tailExtentSq = 0.0f;
        for (const auto& seg : tail)
        {
            Vector2 segCenter = { seg.x + ownerSize.x * 0.5f, seg.y + ownerSize.y * 0.5f };
            float d2 = Vector2DistanceSqr(ownerCenter, segCenter);
            if (d2 > tailExtentSq) tailExtentSq = d2;
        }
        float tailExtent = sqrtf(tailExtentSq);

        float ownerRadius = std::max(ownerSize.x, ownerSize.y) * 0.5f;
        float broadDist = headRadius + ownerRadius + tailExtent + 1.0f;
        if (Vector2DistanceSqr(headCenter, ownerCenter) > broadDist * broadDist) continue;

        // self-collision: skip recent sampled points likely overlapping the head
        const float assumedSampleDistance = 3.0f;
        size_t skipLast = 1;
        if (headRadius > 0.0f)
        {
            skipLast = static_cast<size_t>(ceil(headRadius / assumedSampleDistance)) + 10;
        }

        size_t checkCount = tail.size();
        size_t endIndex = checkCount;
        size_t startIndex = 0;
        if (owner == subject && checkCount > skipLast) endIndex = checkCount - skipLast;
        else if (owner == subject && checkCount > 0) endIndex = 0; // skip all if tail smaller than skip

        for (size_t i = startIndex; i < endIndex; ++i)
        {
            Vector2 seg = tail[i];
            Vector2 segCenter = { seg.x + ownerSize.x * 0.5f, seg.y + ownerSize.y * 0.5f };
            float dx = headCenter.x - segCenter.x;
            float dy = headCenter.y - segCenter.y;
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

    if (state_ == State::Running)
    {
        // update players
        playerOne_.mPHandleInput(dt);
        playerOne_.mPUpdate(dt);

        playerTwo_.mPHandleInput(dt);
        playerTwo_.mPUpdate(dt);

        // if invul expired perform collision checks
        if (spawnInvulnerableTimer_ <= 0.0f)
        {
            if (playerOne_.IsAlive() && CheckHeadAgainstAllTails(&playerOne_)) playerOne_.mPKill();
            if (playerTwo_.IsAlive() && CheckHeadAgainstAllTails(&playerTwo_)) playerTwo_.mPKill();
        }

        // determine game over
        if (!playerOne_.IsAlive() || !playerTwo_.IsAlive())
        {
            state_ = State::GameOver;
            if (playerOne_.IsAlive() && !playerTwo_.IsAlive()) winner_ = 1;
            else if (!playerOne_.IsAlive() && playerTwo_.IsAlive()) winner_ = 2;
            else winner_ = 0;
        }
    }
    else // GameOver: restart on movement or Enter/Space
    {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D) ||
            IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
        {
            ResetPlayers();
            // owners_ rebuilt in ResetPlayers
            state_ = State::Running;
            winner_ = 0;
        }
    }
}

void GameManager::Draw()
{
    // draw players
    playerOne_.mPDraw();
    playerTwo_.mPDraw();

    // debug overlay
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

    // GameOver overlay
    if (state_ == State::GameOver)
    {
        std::string msg;
        if (winner_ == 1) msg = "Player ONE WINS! Move to restart";
        else if (winner_ == 2) msg = "Player TWO WINS! Move to restart";
        else msg = "DRAW! Move to restart";

        int fontSize = 40;
        int textWidth = MeasureText(msg.c_str(), fontSize);
        DrawText(msg.c_str(), (screenWidth_ - textWidth) / 2, screenHeight_ / 2 - 20, fontSize, RAYWHITE);

        std::string hint = "Press Left/Right arrows, A/D, Enter or Space to restart";
        int hintSize = 20;
        int hintW = MeasureText(hint.c_str(), hintSize);
        DrawText(hint.c_str(), (screenWidth_ - hintW) / 2, screenHeight_ / 2 + 30, hintSize, LIGHTGRAY);
    }
}