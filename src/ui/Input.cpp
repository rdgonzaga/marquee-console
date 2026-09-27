#include "ui/Input.h"

#include <conio.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>

#include "core/Commands.h"

namespace input {

namespace {

constexpr int KEY_ENTER     = '\r';
constexpr int KEY_BACKSPACE = 8;
constexpr int KEY_TAB       = '\t';
constexpr int KEY_ESCAPE    = 27;
// extended keys arrive as 0 or 224 followed by one of these
constexpr int KEY_UP        = 72;
constexpr int KEY_DOWN      = 80;
constexpr int KEY_PAGE_UP   = 73;
constexpr int KEY_PAGE_DOWN = 81;
constexpr int SCROLL_STEP   = 5;

void autocomplete(Marquee& m) {
    if (m.input.find(' ') != std::string::npos) return;
    const commands::Command* match = nullptr;
    for (const commands::Command& c : commands::all()) {
        if (std::string(c.name).rfind(m.input, 0) == 0) {
            if (match) return;  // ambiguous
            match = &c;
        }
    }
    if (match) m.input = std::string(match->name) + " ";
}

void handleExtendedKey(Marquee& m, int key) {
    std::lock_guard<std::mutex> lock(m.mtx);
    switch (key) {
        case KEY_UP:
            if (m.historyPos > 0) m.input = m.history[--m.historyPos];
            break;
        case KEY_DOWN:
            if (m.historyPos < m.history.size()) ++m.historyPos;
            m.input = m.historyPos < m.history.size() ? m.history[m.historyPos] : "";
            break;
        case KEY_PAGE_UP:
            m.logScroll = std::min(m.logScroll + SCROLL_STEP, static_cast<int>(m.log.size()));
            break;
        case KEY_PAGE_DOWN:
            m.logScroll = std::max(m.logScroll - SCROLL_STEP, 0);
            break;
    }
}

void handleKey(Marquee& m, int key) {
    if (key == 0 || key == 224) {
        handleExtendedKey(m, _getch());
        return;
    }

    if (key == KEY_ENTER) {
        std::string line;
        {
            std::lock_guard<std::mutex> lock(m.mtx);
            line.swap(m.input);
            if (!line.empty()) {
                addLog(m, LogKind::Command, line);
                m.history.push_back(line);
            }
            m.historyPos = m.history.size();
        }
        commands::execute(m, line);  // commands take the lock themselves
        return;
    }

    std::lock_guard<std::mutex> lock(m.mtx);
    if (key == KEY_BACKSPACE) {
        if (!m.input.empty()) m.input.pop_back();
    } else if (key == KEY_ESCAPE) {
        m.input.clear();
    } else if (key == KEY_TAB) {
        autocomplete(m);
    } else if (key >= 32 && key <= 126 && static_cast<int>(m.input.size()) < config::MAX_INPUT_CHARS) {
        m.input += static_cast<char>(key);
    }
}

}

// getline would block here and freeze the marquee, so poll instead
void loop(Marquee& m) {
    while (!m.quit) {
        while (_kbhit() && !m.quit) handleKey(m, _getch());
        std::this_thread::sleep_for(std::chrono::milliseconds(config::POLL_INTERVAL_MS));
    }
}

}
