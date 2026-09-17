#include "Commands.h"

#include <conio.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <thread>

namespace commands {

namespace {

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    return s.substr(start, s.find_last_not_of(" \t") - start + 1);
}

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

void cmdHelp(Marquee& m, const std::string&) {
    std::lock_guard<std::mutex> lock(m.mtx);
    addLog(m, LogKind::Info, "Available commands:");
    for (const Command& c : all()) {
        std::string usage = c.usage;
        usage.resize(std::max<size_t>(usage.size() + 2, 20), ' ');
        addLog(m, LogKind::Info, "  " + usage + c.description);
    }
    addLog(m, LogKind::Info, "Keys: Up/Down history, Tab complete, PgUp/PgDn scroll, Esc clear line");
}

void cmdStartMarquee(Marquee& m, const std::string&) {
    std::lock_guard<std::mutex> lock(m.mtx);
    if (m.running) {
        addLog(m, LogKind::Info, "Marquee is already running.");
        return;
    }
    m.running = true;
    addLog(m, LogKind::Info, "Marquee started.");
    m.wake.notify_all();
}

void cmdStopMarquee(Marquee& m, const std::string&) {
    std::lock_guard<std::mutex> lock(m.mtx);
    if (!m.running) {
        addLog(m, LogKind::Info, "Marquee is already stopped.");
        return;
    }
    m.running = false;
    addLog(m, LogKind::Info, "Marquee stopped.");
}

void cmdSetText(Marquee& m, const std::string& args) {
    std::string text = args;
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') text = text.substr(1, text.size() - 2);

    std::lock_guard<std::mutex> lock(m.mtx);
    if (text.empty()) {
        addLog(m, LogKind::Error, "Usage: set_text <text>");
        return;
    }
    setText(m, text);
    addLog(m, LogKind::Info, "Marquee text set to \"" + text + "\".");
}

void cmdSetSpeed(Marquee& m, const std::string& args) {
    bool valid = !args.empty() && args.size() <= 6 &&
                 std::all_of(args.begin(), args.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
    int ms = valid ? std::stoi(args) : 0;

    std::lock_guard<std::mutex> lock(m.mtx);
    if (!valid || ms < config::MIN_SPEED_MS || ms > config::MAX_SPEED_MS) {
        addLog(m, LogKind::Error, "Speed must be a whole number from " + std::to_string(config::MIN_SPEED_MS) +
                                      " to " + std::to_string(config::MAX_SPEED_MS) + " (milliseconds).");
        return;
    }
    m.speedMs = ms;
    addLog(m, LogKind::Info, "Marquee speed set to " + std::to_string(ms) + "ms per step.");
    m.wake.notify_all();
}

void cmdExit(Marquee& m, const std::string&) {
    m.quit = true;
}

// --- keyboard ---

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
    const Command* match = nullptr;
    for (const Command& c : all()) {
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
        execute(m, line);  // commands take the lock themselves
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

const std::vector<Command>& all() {
    static const std::vector<Command> table = {
        {"help",          "help",            "Display the commands and their descriptions", cmdHelp},
        {"start_marquee", "start_marquee",   "Start the marquee animation",                 cmdStartMarquee},
        {"stop_marquee",  "stop_marquee",    "Stop the marquee animation",                  cmdStopMarquee},
        {"set_text",      "set_text <text>", "Display the given text as a marquee",         cmdSetText},
        {"set_speed",     "set_speed <ms>",  "Set the animation refresh in milliseconds",   cmdSetSpeed},
        {"exit",          "exit",            "Terminate the console",                       cmdExit},
    };
    return table;
}

void execute(Marquee& m, const std::string& line) {
    std::string trimmed = trim(line);
    if (trimmed.empty()) return;

    size_t space = trimmed.find(' ');
    std::string name = toLower(trimmed.substr(0, space));
    std::string args = space == std::string::npos ? "" : trim(trimmed.substr(space + 1));

    for (const Command& command : all()) {
        if (name == command.name) {
            command.run(m, args);
            return;
        }
    }

    std::lock_guard<std::mutex> lock(m.mtx);
    addLog(m, LogKind::Error, "Unknown command '" + name + "'. Type 'help' to see available commands.");
}

// Polls instead of using getline, so the marquee keeps animating while typing.
void inputLoop(Marquee& m) {
    while (!m.quit) {
        while (_kbhit() && !m.quit) handleKey(m, _getch());
        std::this_thread::sleep_for(std::chrono::milliseconds(config::POLL_INTERVAL_MS));
    }
}

}
