#pragma once
// ============================================================
//  Draw.h
//  Every SDL2 drawing call lives here.
//  No game logic — only "how things look".
// ============================================================
#include "Constants.h"
#include "Logic.h"
#include "Leaderboard.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <cmath>
#include <algorithm>
#include <unordered_map>

// ── Colour helpers ────────────────────────────────────────────
static void sdlCol(SDL_Renderer* r, Col c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}
static Col lerp(Col a, Col b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return { (Uint8)(a.r+(b.r-a.r)*t),
             (Uint8)(a.g+(b.g-a.g)*t),
             (Uint8)(a.b+(b.b-a.b)*t), 255 };
}

// Filled circle using horizontal scanlines
static void fillCircle(SDL_Renderer* r, int cx, int cy, int rad) {
    for (int dy = -rad; dy <= rad; dy++) {
        int dx = (int)std::sqrt((double)(rad*rad - dy*dy));
        SDL_RenderDrawLine(r, cx-dx, cy+dy, cx+dx, cy+dy);
    }
}

// Convert grid cell to screen SDL_Rect
static SDL_Rect cellRect(int col, int row) {
    return { FIELD_X + col*CELL, FIELD_Y + row*CELL, CELL, CELL };
}

// ============================================================
//  Draw  — the main renderer class
// ============================================================
class Draw {
public:
    int tick = 0;

    explicit Draw(SDL_Renderer* ren) : _ren(ren) {
        TTF_Init();
    }
    ~Draw() {
        for (auto& [k,f] : _fonts) if(f) TTF_CloseFont(f);
        TTF_Quit();
    }

    void background() {
        sdlCol(_ren, C_BG);
        SDL_RenderClear(_ren);
    }

    void field() {
        SDL_Rect fr = { FIELD_X, FIELD_Y, FIELD_W, FIELD_H };
        sdlCol(_ren, C_FIELD);
        SDL_RenderFillRect(_ren, &fr);
        sdlCol(_ren, C_GRID);
        for (int c = 0; c <= COLS; c++) {
            int x = FIELD_X + c*CELL;
            SDL_RenderDrawLine(_ren, x, FIELD_Y, x, FIELD_Y+FIELD_H);
        }
        for (int r = 0; r <= ROWS; r++) {
            int y = FIELD_Y + r*CELL;
            SDL_RenderDrawLine(_ren, FIELD_X, y, FIELD_X+FIELD_W, y);
        }
        float p = 0.5f + 0.5f*sinf(tick * 0.04f);
        sdlCol(_ren, lerp(C_BORDER, C_GOLD, p * 0.35f));
        SDL_RenderDrawRect(_ren, &fr);
        SDL_Rect fr2 = { fr.x+1, fr.y+1, fr.w-2, fr.h-2 };
        SDL_RenderDrawRect(_ren, &fr2);
    }

    void snake(const std::deque<Point>& body, Dir dir) {
        int n = (int)body.size();
        for (int i = 0; i < n; i++) {
            float t   = (n > 1) ? (float)i/(n-1) : 0.f;
            Col   seg = lerp(C_HEAD, C_TAIL, t);
            SDL_Rect r = cellRect(body[i].x, body[i].y);
            SDL_Rect inner = { r.x+1, r.y+1, r.w-2, r.h-2 };
            sdlCol(_ren, seg);
            SDL_RenderFillRect(_ren, &inner);
        }
        if (!body.empty()) _drawEyes(body.front(), dir);
    }

    void items(const std::unordered_map<Point,Item,PointHash>& worldItems) {
        for (auto& [pos, kind] : worldItems) {
            SDL_Rect r = cellRect(pos.x, pos.y);
            int cx = r.x + r.w/2,  cy = r.y + r.h/2;

            if (kind == Item::FOOD) {
                float p   = 0.5f + 0.5f*sinf(tick*0.07f);
                int   rad = (int)(6 + 2*p);
                sdlCol(_ren, C_FOOD);
                fillCircle(_ren, cx, cy, rad);
                SDL_SetRenderDrawColor(_ren, 255,210,210,255);
                fillCircle(_ren, cx-2, cy-2, 2);
            } else if (kind == Item::POISON) {
                float p   = 0.5f + 0.5f*sinf(tick*0.07f);
                int   rad = (int)(5 + 2*p);
                sdlCol(_ren, C_POISON);
                fillCircle(_ren, cx, cy, rad);
                SDL_SetRenderDrawColor(_ren, 255,255,255,200);
                SDL_RenderDrawLine(_ren, cx-3,cy-3, cx+3,cy+3);
                SDL_RenderDrawLine(_ren, cx+3,cy-3, cx-3,cy+3);
            } else if (kind == Item::OBSTACLE) {
                SDL_Rect blk = { r.x+2, r.y+2, r.w-4, r.h-4 };
                sdlCol(_ren, C_OBSTACLE);
                SDL_RenderFillRect(_ren, &blk);
                SDL_SetRenderDrawColor(_ren, 110,130,150,255);
                SDL_RenderDrawRect(_ren, &blk);
            }
        }
    }

    void sidebar(int score, int length, int level,
                 const std::string& playerName,
                 const Leaderboard& lb) {
        int sx = SIDE_X, y = FIELD_Y + 8;
        SDL_Rect panel = { sx-8, FIELD_Y-8, SIDE_W+8, FIELD_H+16 };
        sdlCol(_ren, C_PANEL);
        SDL_RenderFillRect(_ren, &panel);
        sdlCol(_ren, C_BORDER);
        SDL_RenderDrawRect(_ren, &panel);

        auto text = [&](const std::string& t, int sz, Col c) {
            auto* f = _font(sz);
            if (!f) { y += sz + 6; return; }
            auto* s = TTF_RenderText_Blended(f, t.c_str(), {c.r,c.g,c.b,c.a});
            if (!s) { y += sz + 6; return; }
            auto* tx = SDL_CreateTextureFromSurface(_ren, s);
            SDL_Rect dst = { sx, y, s->w, s->h };
            SDL_RenderCopy(_ren, tx, nullptr, &dst);
            y += s->h + 4;
            SDL_FreeSurface(s); SDL_DestroyTexture(tx);
        };
        auto gap = [&](int px=8){ y += px; };

        text("Player: " + playerName, 13, C_ACCENT);
        gap(6);
        text("SCORE", 10, C_DIM);
        float p = 0.5f + 0.5f*sinf(tick*0.08f);
        text(std::to_string(score), 22, lerp(C_GOLD, C_RED, p));
        gap(4);
        text("Level:  " + std::to_string(level),  12, C_GOLD);
        text("Length: " + std::to_string(length), 12, C_TEXT);
        gap(8);

        text("GROWTH", 10, C_DIM);
        int barW = SIDE_W - 4, barH = 10;
        SDL_Rect bg   = { sx, y, barW, barH };
        sdlCol(_ren, C_GRID);
        SDL_RenderFillRect(_ren, &bg);
        float pct = std::clamp((float)length / (COLS*ROWS), 0.f, 1.f);
        if (pct > 0) {
            SDL_Rect fill = { sx, y, (int)(barW*pct), barH };
            sdlCol(_ren, C_ACCENT);
            SDL_RenderFillRect(_ren, &fill);
        }
        y += barH + 6;
        gap(8);

        text("CONTROLS", 10, C_DIM);
        for (auto& ln : { "Arrows/WASD move", "P  pause",
                           "R  restart", "ESC  menu" })
            text(ln, 10, C_DIM);
        gap(12);

        text("TOP SCORES", 11, C_GOLD);
        sdlCol(_ren, C_GOLD);
        SDL_RenderDrawLine(_ren, sx, y, sx+barW, y);
        y += 6;
        auto& entries = lb.all();
        int show = std::min((int)entries.size(), 5);
        for (int i = 0; i < show; i++) {
            Col rc = (i==0) ? C_GOLD : (i<3) ? C_TEXT : C_DIM;
            char buf[48];
            snprintf(buf, sizeof(buf), "#%d %-9s %5d",
                     i+1, entries[i].name.substr(0,9).c_str(), entries[i].score);
            text(buf, 10, rc);
        }
    }

    void titleBar(bool paused) {
        std::string t = paused ? "[ PAUSED ]" : "SNAKE";
        Col c = paused ? C_GOLD : C_ACCENT;
        _text(t, 20, c, FIELD_X, FIELD_Y - 36);
    }

    void overlayGameOver(int score, int rank) {
        _dimOverlay();
        int cx = FIELD_X + FIELD_W/2, cy = FIELD_Y + FIELD_H/2;
        _textC("GAME OVER",  38, C_RED,    cx, cy - 55);
        _textC("Score: " + std::to_string(score), 20, C_GOLD, cx, cy);
        _textC("Rank #" + std::to_string(rank),   14, C_TEXT, cx, cy + 35);
        _textC("R = restart     ESC = menu", 12, C_DIM, cx, cy + 68);
    }

    void overlayWin(int score) {
        _dimOverlay();
        int cx = FIELD_X + FIELD_W/2, cy = FIELD_Y + FIELD_H/2;
        float p = 0.5f + 0.5f*sinf(tick*0.12f);
        _textC("YOU WIN!",   42, lerp(C_ACCENT, C_GOLD, p), cx, cy - 50);
        _textC("Score: " + std::to_string(score), 20, C_GOLD, cx, cy + 10);
        _textC("R = restart     ESC = menu", 12, C_DIM, cx, cy + 52);
    }

    void pauseBanner() {
        _textC("PAUSED", 28, C_GOLD,
               FIELD_X + FIELD_W/2, FIELD_Y + FIELD_H/2);
    }

private:
    SDL_Renderer* _ren;
    std::unordered_map<int, TTF_Font*> _fonts;

    // ── Font loader — tries Windows fonts first, then MSYS2, then Linux ──
    TTF_Font* _font(int sz) {
        auto it = _fonts.find(sz);
        if (it != _fonts.end()) return it->second;
        const char* paths[] = {
            // Windows system fonts
            "C:/Windows/Fonts/consola.ttf",   // Consolas
            "C:/Windows/Fonts/cour.ttf",      // Courier New
            "C:/Windows/Fonts/lucon.ttf",     // Lucida Console
            "C:/Windows/Fonts/arial.ttf",     // Arial (fallback)
            "C:/Windows/Fonts/calibri.ttf",   // Calibri (fallback)
            // MSYS2 bundled fonts
            "C:/msys64/usr/share/fonts/TTF/DejaVuSansMono.ttf",
            "C:/msys64/usr/share/fonts/TTF/DejaVuSans.ttf",
            // Linux fallbacks
            "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
            nullptr
        };
        TTF_Font* f = nullptr;
        for (int i = 0; paths[i] && !f; i++) f = TTF_OpenFont(paths[i], sz);
        return _fonts[sz] = f;
    }

    void _text(const std::string& t, int sz, Col c, int x, int y) {
        auto* f = _font(sz);
        if (!f) return;
        auto* s  = TTF_RenderText_Blended(f, t.c_str(), {c.r,c.g,c.b,c.a});
        if (!s) return;
        auto* tx = SDL_CreateTextureFromSurface(_ren, s);
        SDL_Rect dst = { x, y, s->w, s->h };
        SDL_RenderCopy(_ren, tx, nullptr, &dst);
        SDL_FreeSurface(s);
        SDL_DestroyTexture(tx);
    }

    void _textC(const std::string& t, int sz, Col c, int cx, int cy) {
        auto* f = _font(sz);
        if (!f) return;
        int w=0, h=0;
        TTF_SizeText(f, t.c_str(), &w, &h);
        _text(t, sz, c, cx - w/2, cy - h/2);
    }

    void _drawEyes(Point head, Dir dir) {
        SDL_Rect r = cellRect(head.x, head.y);
        int cx = r.x+r.w/2, cy = r.y+r.h/2;
        auto [dx,dy] = dirVec(dir);
        int px = -dy, py = dx;
        for (int s : {-1, 1}) {
            int ex = cx + dx*4 + px*s*4;
            int ey = cy + dy*4 + py*s*4;
            SDL_SetRenderDrawColor(_ren, 255,255,255,255);
            fillCircle(_ren, ex, ey, 3);
            SDL_SetRenderDrawColor(_ren, 15,15,15,255);
            fillCircle(_ren, ex+dx, ey+dy, 1);
        }
    }

    void _dimOverlay() {
        SDL_SetRenderDrawBlendMode(_ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(_ren, 0,0,0,165);
        SDL_Rect full = { 0,0,SCREEN_W,SCREEN_H };
        SDL_RenderFillRect(_ren, &full);
        SDL_SetRenderDrawBlendMode(_ren, SDL_BLENDMODE_NONE);
    }
};
