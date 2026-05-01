// ============================================================
//  main.cpp
//  Entry point. Contains:
//    • runMenu()     — name entry + leaderboard screen
//    • runGame()     — the actual snake game loop
//    • main()        — SDL2 setup, outer menu↔game loop
//
//  Flow:
//    main() → runMenu() → runGame() → back to runMenu()
// ============================================================
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include "Constants.h"
#include "Logic.h"
#include "Leaderboard.h"
#include "Sound.h"
#include "Draw.h"

#include <string>
#include <memory>
#include <deque>
#include <random>
#include <cmath>
#include <iostream>

// ============================================================
//  MENU
//  Returns the player name, or "" if the user wants to quit.
// ============================================================
std::string runMenu(SDL_Renderer* ren, Draw& draw,
                    Sound& snd, const Leaderboard& lb) {
    std::string name;
    bool showLB    = false;
    bool cursorVis = true;
    int  cursorTimer = 0;
    int  tick      = 0;
    bool running   = true;
    bool quit      = false;

    SDL_Event ev;
    while (running) {
        // ── Events ───────────────────────────────────────────
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)          { quit = true; running = false; break; }
            if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;

                if (showLB) { showLB = false; snd.play("nav"); continue; }

                if (k == SDLK_ESCAPE)         { quit = true; running = false; break; }
                if (k == SDLK_RETURN && !name.empty()) { running = false; break; }
                if (k == SDLK_BACKSPACE && !name.empty()) {
                    name.pop_back(); snd.play("nav");
                }
                if (k == SDLK_l) { showLB = true; snd.play("nav"); }
                // Accept printable ASCII, max 12 chars
                if (k >= 32 && k < 127 && (int)name.size() < 12) {
                    name += (char)k;
                    snd.play("nav");
                }
            }
        }
        if (quit) return "";

        // ── Cursor blink ─────────────────────────────────────
        cursorTimer++;
        if (cursorTimer >= FPS/2) { cursorTimer = 0; cursorVis = !cursorVis; }
        tick++;

        // ── Draw ─────────────────────────────────────────────
        draw.background();
        draw.tick = tick;

        int cx = SCREEN_W/2;

        // Title
        float p = 0.5f + 0.5f*sinf(tick*0.04f);
        Col titleCol = lerp(C_ACCENT, C_GOLD, p);

        // We'll use the Draw helper via lambda
        // (Draw exposes no generic textCentered, so we replicate a tiny helper here)
        auto textC = [&](const std::string& t, int sz, Col c, int x, int y) {
            // Render via SDL_ttf directly
            static std::unordered_map<int,TTF_Font*> fonts;
            if (!fonts.count(sz)) {
                const char* paths[] = {
                    "C:/Windows/Fonts/consola.ttf",
                    "C:/Windows/Fonts/cour.ttf",
                    "C:/Windows/Fonts/lucon.ttf",
                    "C:/Windows/Fonts/arial.ttf",
                    "C:/msys64/usr/share/fonts/TTF/DejaVuSansMono.ttf",
                    "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
                    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                    nullptr
                };
                TTF_Font* f = nullptr;
                for (int i = 0; paths[i] && !f; i++) f = TTF_OpenFont(paths[i], sz);
                fonts[sz] = f;
            }
            TTF_Font* f = fonts[sz];
            if (!f) return;
            auto* s = TTF_RenderText_Blended(f, t.c_str(), {c.r,c.g,c.b,c.a});
            if (!s) return;
            auto* tx = SDL_CreateTextureFromSurface(ren, s);
            SDL_Rect dst = { x - s->w/2, y - s->h/2, s->w, s->h };
            SDL_RenderCopy(ren, tx, nullptr, &dst);
            SDL_FreeSurface(s);
            SDL_DestroyTexture(tx);
        };

        if (showLB) {
            // ── Leaderboard overlay ───────────────────────────
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren,0,0,0,200);
            SDL_Rect full={0,0,SCREEN_W,SCREEN_H};
            SDL_RenderFillRect(ren,&full);
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_NONE);

            SDL_Rect panel = {cx-230, 70, 460, 520};
            sdlCol(ren, C_PANEL);
            SDL_RenderFillRect(ren, &panel);
            sdlCol(ren, C_GOLD);
            SDL_RenderDrawRect(ren, &panel);

            textC("LEADERBOARD", 24, C_GOLD, cx, 110);

            auto& entries = lb.all();
            int sy = 160;
            int show = std::min((int)entries.size(), 10);
            for (int i = 0; i < show; i++) {
                Col rc = (i==0)?C_GOLD:(i<3)?C_TEXT:C_DIM;
                char buf[48];
                snprintf(buf,sizeof(buf),"#%2d  %-12s  %6d",
                         i+1, entries[i].name.substr(0,12).c_str(), entries[i].score);
                textC(buf, 14, rc, cx, sy + i*36);
            }
            textC("Press any key to close", 11, C_DIM, cx, panel.y+panel.h-22);
        } else {
            // ── Name entry screen ─────────────────────────────
            textC("SNAKE", 54, titleCol, cx, 130);
            textC("Classic Snake — wrap-around edges, poison, obstacles", 12, C_DIM, cx, 192);

            textC("Enter your name:", 16, C_TEXT, cx, 280);

            // Input box
            SDL_Rect box = { cx-140, 302, 280, 44 };
            sdlCol(ren, C_PANEL);
            SDL_RenderFillRect(ren, &box);
            Col borderC = name.empty() ? C_DIM : C_ACCENT;
            sdlCol(ren, borderC);
            SDL_RenderDrawRect(ren, &box);

            std::string disp = name + (cursorVis ? "|" : " ");
            textC(disp, 20, C_ACCENT, cx, 324);

            Col hint = name.empty() ? C_DIM : C_ACCENT;
            textC("Press ENTER to start", 13, hint, cx, 372);

            textC("L = view leaderboard      ESC = quit", 11, C_DIM, cx, SCREEN_H - 32);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(1000/FPS);
    }
    return quit ? "" : name;
}

// ============================================================
//  GAME
//  Returns true to go back to menu, false to quit entirely.
// ============================================================
bool runGame(SDL_Renderer* ren, Draw& draw,
             Sound& snd, Leaderboard& lb,
             const std::string& playerName) {

    // ── State ────────────────────────────────────────────────
    enum class State { PLAYING, DEAD, WIN };

    Snake snake({ COLS/2, ROWS/2 }, 4, Dir::RIGHT);
    World world;
    world.syncFree(snake.bodySet());
    world.ensureFood(2);
    world.ensurePoison(1);
    world.ensureObstacles(3);

    float speed    = INIT_SPEED;  // moves per second
    float moveAcc  = 0;           // accumulated seconds toward next move
    int   level    = 1;
    State state    = State::PLAYING;
    bool  paused   = false;
    bool  saved    = false;       // prevent double-saving score
    std::deque<Dir> dirBuf;       // buffered input (max 2)
    std::mt19937 rng(std::random_device{}());

    int   tick     = 0;
    Uint32 prev    = SDL_GetTicks();
    SDL_Event ev;

    while (true) {
        // ── Delta time ───────────────────────────────────────
        Uint32 now = SDL_GetTicks();
        float  dt  = (now - prev) / 1000.f;
        prev       = now;
        tick++;
        draw.tick  = tick;

        // ── Events ───────────────────────────────────────────
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)    return false;
            if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_ESCAPE)   return true;
                if (k == SDLK_r)        return true;  // restart via menu
                if (k == SDLK_p && state==State::PLAYING) {
                    paused = !paused; snd.play("nav");
                }
                // Direction input — buffer up to 2 pending turns
                Dir d = Dir::RIGHT;
                bool isDir = true;
                switch(k) {
                    case SDLK_UP:    case SDLK_w: d=Dir::UP;    break;
                    case SDLK_DOWN:  case SDLK_s: d=Dir::DOWN;  break;
                    case SDLK_LEFT:  case SDLK_a: d=Dir::LEFT;  break;
                    case SDLK_RIGHT: case SDLK_d: d=Dir::RIGHT; break;
                    default: isDir=false;
                }
                if (isDir && state==State::PLAYING && !paused)
                    if ((int)dirBuf.size() < 2) dirBuf.push_back(d);
            }
        }

        // ── Update (only when playing and not paused) ────────
        if (state == State::PLAYING && !paused) {
            // Apply buffered direction
            if (!dirBuf.empty()) {
                snake.setDir(dirBuf.front());
                dirBuf.pop_front();
            }

            // Accumulate time; move when enough has passed
            moveAcc += dt;
            float interval = 1.f / speed;

            if (moveAcc >= interval) {
                moveAcc -= interval;

                Point newHead = snake.step(0, 0);

                // ── Death: hit self ──────────────────────────
                if (!snake.alive) {
                    snd.play("die");
                    state = State::DEAD;
                    if (!saved) { lb.add(playerName, world.score); saved=true; }
                    goto draw_frame;
                }

                // ── Death: hit obstacle ──────────────────────
                if (world.obstacleSet().count(newHead)) {
                    snd.play("obstacle");
                    snake.alive = false;
                    state = State::DEAD;
                    if (!saved) { lb.add(playerName, world.score); saved=true; }
                    goto draw_frame;
                }

                // ── Item hit ────────────────────────────────
                auto* hitItem = world.hitTest(newHead);
                if (hitItem) {
                    Item kind = *hitItem;
                    world.consume(newHead);

                    if (kind == Item::FOOD) {
                        snd.play("eat");
                        snake.step(1, 0);   // grow by 1
                    } else if (kind == Item::POISON) {
                        snd.play("poison");
                        snake.step(0, POISON_SHRINK);
                        if (snake.len() <= 0) {
                            state = State::DEAD;
                            if (!saved) { lb.add(playerName, world.score); saved=true; }
                            goto draw_frame;
                        }
                    }
                }

                // ── Maintain item counts ─────────────────────
                world.syncFree(snake.bodySet());
                world.ensureFood(2);
                world.ensurePoison(1);
                world.ensureObstacles(3 + level / 2);

                // ── Occasionally spawn a bonus poison ────────
                std::uniform_real_distribution<float> rd(0,1);
                if (rd(rng) < 0.006f) world.ensurePoison(2);

                // ── Speed & level scaling ────────────────────
                level = 1 + world.foodEaten / 5;
                speed = std::min(INIT_SPEED + (level-1)*SPEED_STEP, MAX_SPEED);

                // ── Win condition ────────────────────────────
                if (snake.len() >= COLS*ROWS) {
                    snd.play("win");
                    state = State::WIN;
                    if (!saved) { lb.add(playerName, world.score); saved=true; }
                }
            }
        }

        draw_frame:
        // ── Draw ─────────────────────────────────────────────
        draw.background();
        draw.field();
        draw.items(world.items);
        draw.snake(snake.body, snake.dir);
        draw.sidebar(world.score, snake.len(), level, playerName, lb);
        draw.titleBar(paused);

        if      (state == State::DEAD) draw.overlayGameOver(world.score, lb.rankOf(world.score));
        else if (state == State::WIN)  draw.overlayWin(world.score);
        else if (paused)               draw.pauseBanner();

        SDL_RenderPresent(ren);
        SDL_Delay(1000/FPS);
    }
}

// ============================================================
//  MAIN  — SDL2 setup, outer loop
// ============================================================
int main(int /*argc*/, char** /*argv*/) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }
    TTF_Init();

    SDL_Window* win = SDL_CreateWindow(
        "Snake",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN);

    SDL_Renderer* ren = SDL_CreateRenderer(
        win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    Draw       draw(ren);
    Sound      snd;
    Leaderboard lb;

    // ── Outer menu ↔ game loop ────────────────────────────────
    while (true) {
        std::string player = runMenu(ren, draw, snd, lb);
        if (player.empty()) break;   // user quit from menu

        bool backToMenu = runGame(ren, draw, snd, lb, player);
        if (!backToMenu) break;      // user quit from inside game
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
