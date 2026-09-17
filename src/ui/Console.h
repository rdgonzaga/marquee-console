#pragma once

#include <atomic>
#include <string>

// Everything that talks to the Windows console directly.
namespace console {

struct Size {
    int width;
    int height;
};

// Turns on colors, hides the cursor, and routes Ctrl+C into `quit`.
void init(std::atomic<bool>& quit);
void restore();

Size size();

// Writes in one call, which is what keeps frames from tearing.
void write(const std::string& text);

// ANSI color codes, so the rest of the program never spells them out.
std::string fg(int color);
std::string bg(int color);

inline constexpr const char* RESET = "\x1b[0m";
inline constexpr const char* BOLD  = "\x1b[1m";

}
