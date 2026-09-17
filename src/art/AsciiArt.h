#pragma once

#include <string>
#include <vector>

// All the ASCII art in the program. Everything here is plain text you can
// paste over: the letter shapes, the logo, and the group info.
namespace ascii {

inline constexpr int GLYPH_HEIGHT = 6;

// "hello" -> the big-letter version, one string per row.
std::vector<std::string> render(const std::string& text);

// Splits pasted art into rows.
std::vector<std::string> split(const char* art);

extern const char* const LOGO;
extern const char* const INFO;

}
