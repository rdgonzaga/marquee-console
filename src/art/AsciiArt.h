#pragma once

#include <string>
#include <vector>

// Letter shapes, logo, group info. All of it is plain text to paste over.
namespace ascii {

inline constexpr int GLYPH_HEIGHT = 6;

std::vector<std::string> render(const std::string& text);

std::vector<std::string> split(const char* art);

extern const char* const LOGO;
extern const char* const INFO;

}
