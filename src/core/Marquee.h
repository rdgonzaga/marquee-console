#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "Config.h"

enum class LogKind { Command, Info, Error };

struct LogLine {
    LogKind kind;
    std::string text;
};

// Shared by the three threads. Everything except `quit` needs `mtx` held.
struct Marquee {
    std::mutex mtx;
    std::condition_variable wake;
    std::atomic<bool> quit{false};

    bool running = true;
    int speedMs = config::DEFAULT_SPEED_MS;

    std::string text;
    std::vector<std::string> art;
    int artWidth = 0;

    int x = 0, y = 0;
    int dx = 1, dy = 1;
    int boxWidth = 80;
    int boxHeight = config::BOX_HEIGHT;

    std::deque<LogLine> log;
    int logScroll = 0;
    std::string input;
    std::vector<std::string> history;
    size_t historyPos = 0;
    int fps = 0;
};

// Moves the art one step every speedMs until quit. Runs on its own thread.
void marqueeLoop(Marquee& marquee);

// Both need marquee.mtx held.
void setText(Marquee& marquee, const std::string& text);
void addLog(Marquee& marquee, LogKind kind, const std::string& text);
