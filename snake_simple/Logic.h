#pragma once
// ============================================================
//  Logic.h
//  Contains two classes:
//    • Snake  – the snake body using std::deque
//    • World  – food / poison / obstacles using std::unordered_map
//
//  No SDL drawing here — pure game logic only.
// ============================================================
#include "Constants.h"
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <algorithm>
#include <cstdlib>

// ── Point: a grid cell coordinate ────────────────────────────
struct Point {
    int x, y;
    bool operator==(const Point& o) const { return x==o.x && y==o.y; }
};
struct PointHash {
    size_t operator()(const Point& p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 16);
    }
};
using PointSet = std::unordered_set<Point, PointHash>;

// ── Direction ─────────────────────────────────────────────────
enum class Dir { UP, DOWN, LEFT, RIGHT };

inline Point dirVec(Dir d) {
    switch(d) {
        case Dir::UP:    return { 0,-1};
        case Dir::DOWN:  return { 0, 1};
        case Dir::LEFT:  return {-1, 0};
        case Dir::RIGHT: return { 1, 0};
    }
    return {1,0};
}
inline Dir opposite(Dir d) {
    switch(d) {
        case Dir::UP:    return Dir::DOWN;
        case Dir::DOWN:  return Dir::UP;
        case Dir::LEFT:  return Dir::RIGHT;
        case Dir::RIGHT: return Dir::LEFT;
    }
    return Dir::RIGHT;
}

// ============================================================
//  Snake
//  Body stored in std::deque<Point>:
//    front = head,  back = tail
//  O(1) push_front (new head) and pop_back (tail removal)
// ============================================================
class Snake {
public:
    std::deque<Point> body;
    Dir  dir   = Dir::RIGHT;
    bool alive = true;

    Snake(Point start, int length, Dir startDir)
        : dir(startDir), _queued(startDir)
    {
        // Build initial body going left from start
        for (int i = 0; i < length; i++)
            body.push_back({ ((start.x - i) % COLS + COLS) % COLS, start.y });
    }

    // Buffer one direction change (ignores reversal)
    void setDir(Dir d) {
        if (d != opposite(dir)) _queued = d;
    }

    // Advance one cell.
    // grow   > 0 → add that many segments at the tail
    // shrink > 0 → remove that many segments from the tail
    // Returns the new head position.
    Point step(int grow = 0, int shrink = 0) {
        dir = _queued;

        auto [dx, dy] = dirVec(dir);
        Point newHead = { ((body.front().x + dx) % COLS + COLS) % COLS,
                          ((body.front().y + dy) % ROWS + ROWS) % ROWS };

        // Self-collision: build body set minus the tail (which will move away)
        PointSet bset(body.begin(), body.end());
        if (grow == 0 && body.size() > 1) bset.erase(body.back());

        if (bset.count(newHead)) { alive = false; return body.front(); }

        body.push_front(newHead);

        if (grow <= 0) {
            if (body.size() > 1) body.pop_back();
        } else {
            // Already added head; duplicate tail (grow-1) more times
            for (int i = 0; i < grow - 1; i++) body.push_back(body.back());
        }

        for (int i = 0; i < shrink; i++) {
            if ((int)body.size() > 1) body.pop_back();
            else { alive = false; break; }
        }
        return body.front();
    }

    Point head() const { return body.front(); }
    int   len()  const { return (int)body.size(); }
    PointSet bodySet() const { return PointSet(body.begin(), body.end()); }

private:
    Dir _queued;
};

// ============================================================
//  World
//  Items stored in std::unordered_map<Point, ItemKind>
//  Free cells in std::unordered_set<Point> for O(1) random spawn
// ============================================================
enum class Item { FOOD, POISON, OBSTACLE };

class World {
public:
    std::unordered_map<Point, Item, PointHash> items;
    int score      = 0;
    int foodEaten  = 0;

    World() : _rng(std::random_device{}()) { _rebuildFree({}); }

    // Call each frame so spawn pool stays accurate
    void syncFree(const PointSet& occupied) {
        _free.clear();
        for (int c = 0; c < COLS; c++)
            for (int r = 0; r < ROWS; r++) {
                Point p{c,r};
                if (!occupied.count(p) && !items.count(p))
                    _free.insert(p);
            }
    }

    // Keep at least `n` of each item type on the field
    void ensureFood(int n)     { _fill(Item::FOOD,     n); }
    void ensurePoison(int n)   { _fill(Item::POISON,   n); }
    void ensureObstacles(int n){ _fill(Item::OBSTACLE, n); }

    // Check if snake head hit an item; returns nullptr if nothing there
    const Item* hitTest(Point p) const {
        auto it = items.find(p);
        return it == items.end() ? nullptr : &it->second;
    }

    // Remove item at p. Returns {score_delta, grow_delta}
    std::pair<int,int> consume(Point p) {
        auto it = items.find(p);
        if (it == items.end()) return {0,0};
        Item k = it->second;
        items.erase(it);
        _free.insert(p);
        if (k == Item::FOOD)   { score += FOOD_PTS;  foodEaten++; return {FOOD_PTS,  1}; }
        if (k == Item::POISON) { score  = std::max(0, score - POISON_PTS); return {-POISON_PTS, -POISON_SHRINK}; }
        return {0, 0};
    }

    PointSet obstacleSet() const {
        PointSet s;
        for (auto& [p,k] : items) if (k == Item::OBSTACLE) s.insert(p);
        return s;
    }

private:
    std::unordered_set<Point, PointHash> _free;
    std::mt19937 _rng;

    void _rebuildFree(const PointSet& occ) {
        _free.clear();
        for (int c = 0; c < COLS; c++)
            for (int r = 0; r < ROWS; r++) {
                Point p{c,r};
                if (!occ.count(p)) _free.insert(p);
            }
    }

    void _spawn(Item k) {
        if (_free.empty()) return;
        std::uniform_int_distribution<int> d(0, (int)_free.size()-1);
        auto it = _free.begin();
        std::advance(it, d(_rng));
        Point p = *it;
        _free.erase(it);
        items[p] = k;
    }

    void _fill(Item k, int minCount) {
        int cur = 0;
        for (auto& [p,i] : items) if (i==k) cur++;
        for (int i = 0; i < minCount - cur; i++) _spawn(k);
    }
};
