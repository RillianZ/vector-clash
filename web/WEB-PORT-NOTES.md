# Vector Clash: web port notes

This folder is a **copy** of the game source from
`GDrive\Studio and Development\Finished Games or proof of concepts\Vector clash\NoahVectorClash\src`.
Your original folder is untouched. All changes are marked in the code with `// [web-port]` or `// [gamepad]`.

## Changes for the browser (only active when built with PLATFORM_WEB)
- **main.cpp**: `SetExitKey(KEY_NULL)`: in a browser, Esc would end the main loop and freeze the page.
- **GameManager.cpp: music**: the six background tracks are not packed into the game. The web page
  downloads them one by one *after* the game starts (track 1 first), and the game picks each one up as it
  arrives (`FileExists` check in `Update`). The first download is ~1.3 MB instead of ~65 MB.
  T only cycles through tracks that have arrived. Tracks are re-encoded to 48 kbps mono MP3 for the web.
- **GameManager.cpp: high scores**: `SaveHighScores` / `LoadHighScores` use the browser's localStorage
  (key `vector-clash-best`) instead of `vector_clash_save.txt`.
- **Build**: needs a 1 MB stack (`-sSTACK_SIZE=1048576`); Emscripten's 64 KB default crashed on load.

## New: gamepad support (works on desktop too: not behind PLATFORM_WEB)
- **mPlayer**: `SetGamepad(index)`; gamepad N steers player N+1. D-pad = full turn, left stick = proportional
  (25% deadzone, same max turn rate as the keys). Keyboard still works at the same time.
- **Menu**: any pad: A/Start = start, D-pad up/down = player count, X = difficulty, Y = map, Back = music.
  Each pad: D-pad left/right or LB/RB = cycle that player's bike. Boxes show "+ GAMEPAD N" when connected,
  and a gamepad help line appears when any pad is plugged in.
- **Game over**: A/Start = retry, B/Back = menu.

To use gamepads in the desktop build, copy `mPlayer.h`, `mPlayer.cpp` and `GameManager.cpp` over your
originals (keep a backup): the `#if defined(PLATFORM_WEB)` parts are skipped on Windows automatically.

## Build command (Emscripten 3.1.50 + raylib 5.0 built for PLATFORM_WEB)
    em++ -O2 -std=c++17 -DPLATFORM_WEB -I<raylib/src> src/*.cpp <raylib/src/libraylib.a>
      -sUSE_GLFW=3 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=67108864 -sSTACK_SIZE=1048576
      -sEXIT_RUNTIME=0 -sFORCE_FILESYSTEM=1 -sEXPORTED_RUNTIME_METHODS=FS
      --embed-file res/resources@resources -o vectorclash.js
`res/resources` = the sound effects only (no background*.mp3). Music files go in `music/t1.bin ... t6.bin`:
an 8-byte header `VCTRK1\n\0` followed by the MP3 (the header stops file-sync tools from rewriting audio).

## Spawn shield fix (2026-10-07, asked for by Noah: applies to desktop too)
- The old 1 s protection ran out during the 3 s "GET READY" countdown, so it never did anything
  (e.g. a random-map block spawning on top of a player killed them instantly).
- Now `spawnInvulnerableSeconds_ = 4` and the timer only counts down in `UpdateRunning` *after* the countdown.
- While shielded, walls, blocks and trails can't kill anyone, but leaving the arena still does
  (otherwise a fast bike could drive straight out through the 10 px border).
- A pulsing ring around every head + "SPAWN SHIELD 3/2/1" at the top shows it's active.

## Noted and left as is (Noah: intentional)
- "Map: FIXED" label wording, the solo debug HUD, and the map preview behind the menu.
- Player count shown twice in the menu; Esc quits the desktop build.
