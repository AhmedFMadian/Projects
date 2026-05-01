#pragma once
// ============================================================
//  Leaderboard.h
//  Keeps the top MAX_ENTRIES scores sorted highest first.
//  Data structure: std::vector<Entry> — sorted after each insert.
//  Persistence   : hand-written minimal JSON (no external lib).
// ============================================================
#include "Constants.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

struct Entry {
    std::string name;
    int         score;
};

class Leaderboard {
public:
    Leaderboard() { _load(); }

    // Add a score and save to disk
    void add(const std::string& name, int score) {
        _data.push_back({ name.empty() ? "Anon" : name.substr(0,12), score });
        std::sort(_data.begin(), _data.end(),
                  [](const Entry& a, const Entry& b){ return a.score > b.score; });
        if ((int)_data.size() > MAX_ENTRIES) _data.resize(MAX_ENTRIES);
        _save();
    }

    // 1-based rank the given score would occupy
    int rankOf(int score) const {
        for (int i = 0; i < (int)_data.size(); i++)
            if (score >= _data[i].score) return i + 1;
        return (int)_data.size() + 1;
    }

    const std::vector<Entry>& all() const { return _data; }

private:
    std::vector<Entry> _data;

    // ── JSON write ───────────────────────────────────────────
    void _save() const {
        std::ofstream f(LB_FILE);
        if (!f) return;
        f << "[\n";
        for (int i = 0; i < (int)_data.size(); i++) {
            f << "  {\"name\":\"" << _data[i].name
              << "\",\"score\":"  << _data[i].score << "}";
            if (i + 1 < (int)_data.size()) f << ",";
            f << "\n";
        }
        f << "]\n";
    }

    // ── JSON read (simple line-by-line parser) ───────────────
    void _load() {
        std::ifstream f(LB_FILE);
        if (!f) return;
        std::string line;
        while (std::getline(f, line)) {
            // Look for lines that contain both "name" and "score"
            auto np = line.find("\"name\":\"");
            auto sp = line.find("\"score\":");
            if (np == std::string::npos || sp == std::string::npos) continue;
            try {
                np += 8;
                std::string name = line.substr(np, line.find('"', np) - np);
                int score = std::stoi(line.substr(sp + 8));
                _data.push_back({name, score});
            } catch(...) {}
        }
        std::sort(_data.begin(), _data.end(),
                  [](const Entry& a, const Entry& b){ return a.score > b.score; });
        if ((int)_data.size() > MAX_ENTRIES) _data.resize(MAX_ENTRIES);
    }
};
