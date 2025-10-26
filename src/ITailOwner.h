#pragma once
#include <deque>
#include <raylib.h>

// Minimal interface for anything that exposes a tail and basic head parameters.
// Implemented by players and later by enemies.

//THIS IS A COPIED CLASS LOOK INTO HOW THIS WORKS WITH VIRTUAL!!!!
class ITailOwner
{
public:
    virtual ~ITailOwner() = default;
    virtual Vector2 GetPosition() const = 0;                    // top-left of head
    virtual Vector2 GetSize() const = 0;                        // head size
    virtual const std::deque<Vector2>& GetTailSegments() const = 0;
    virtual bool IsAlive() const = 0;
};