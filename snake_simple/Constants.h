#pragma once
// ============================================================
//  Constants.h
//  All game-wide settings live here.
//  Change a number here and it affects the whole game.
// ============================================================
#include <SDL2/SDL.h>

// ── Window ───────────────────────────────────────────────────
const int SCREEN_W  = 860;
const int SCREEN_H  = 660;

// ── Grid / Playfield ─────────────────────────────────────────
const int CELL      = 20;       // pixels per grid cell
const int COLS      = 28;       // number of columns
const int ROWS      = 28;       // number of rows
const int FIELD_X   = 30;       // left edge of playfield
const int FIELD_Y   = 50;       // top  edge of playfield
const int FIELD_W   = COLS * CELL;   // 560 px
const int FIELD_H   = ROWS * CELL;   // 560 px

// ── Sidebar ───────────────────────────────────────────────────
const int SIDE_X    = FIELD_X + FIELD_W + 20;  // sidebar left edge
const int SIDE_W    = SCREEN_W - SIDE_X - 10;  // sidebar width

// ── Timing ───────────────────────────────────────────────────
const int   FPS           = 60;
const float INIT_SPEED    = 6.0f;   // moves per second at start
const float MAX_SPEED     = 16.0f;  // moves per second cap
const float SPEED_STEP    = 0.5f;   // speed added every 5 foods eaten

// ── Scoring ──────────────────────────────────────────────────
const int FOOD_PTS    = 10;
const int POISON_PTS  = 5;      // penalty when eating poison
const int POISON_SHRINK = 2;    // segments removed on poison

// ── Leaderboard ──────────────────────────────────────────────
const int  MAX_ENTRIES   = 10;
const char LB_FILE[]     = "scores.json";

// ── Simple colour struct ──────────────────────────────────────
struct Col { Uint8 r, g, b, a = 255; };

// ── Palette ──────────────────────────────────────────────────
const Col C_BG       = {10,  12,  20};
const Col C_FIELD    = {15,  18,  30};
const Col C_GRID     = {22,  26,  42};
const Col C_BORDER   = {0,  200, 150};
const Col C_TEXT     = {220, 230, 255};
const Col C_DIM      = {100, 115, 145};
const Col C_ACCENT   = {0,  210, 140};
const Col C_GOLD     = {255, 185,  50};
const Col C_RED      = {255,  60,  60};
const Col C_PANEL    = {18,  22,  38};
const Col C_FOOD     = {255,  80,  80};
const Col C_POISON   = {160,  30, 200};
const Col C_OBSTACLE = {70,   90, 110};
const Col C_HEAD     = {0,   220, 120};
const Col C_TAIL     = {0,    90,  45};
