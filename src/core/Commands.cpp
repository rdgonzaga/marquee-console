#include "core/Commands.h"

#include <algorithm>
#include <cctype>

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

}
