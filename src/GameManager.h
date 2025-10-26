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

    // Reset players / restart match
    void ResetPlayers();

private:
    int screenWidth_;
    int screenHeight_;
    float halfScreenWidth_;
    float halfScreenHeight_;

    // spawn invulnerability window after reset
    float spawnInvulnerableSeconds_;
    float spawnInvulnerableTimer_;

    enum class State { Running, GameOver };
    State state_;
    int winner_; // 0 none/draw, 1 player1, 2 player2

    // Two players for prototype; later make a container or factory
    mPlayer playerOne_;
    mPlayer playerTwo_;

    // Tail owners list for generic collision checks
    std::vector<ITailOwner*> owners_;

    // internal routines
    bool CheckHeadAgainstAllTails(ITailOwner* subject);
};
