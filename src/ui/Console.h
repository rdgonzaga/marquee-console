#pragma once

#include <atomic>
#include <string>

// The only file that talks to Windows directly.
namespace console {

struct Size {
    int width;
    int height;
};

void init(std::atomic<bool>& quit);
void restore();

Size size();

// one call per frame, otherwise it tears
void write(const std::string& text);

std::string fg(int color);
std::string bg(int color);

inline constexpr const char* RESET = "\x1b[0m";
inline constexpr const char* BOLD  = "\x1b[1m";

}
