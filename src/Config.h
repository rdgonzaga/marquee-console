#pragma once

// Every setting worth changing lives here.
namespace config {

// timing
inline constexpr int RENDER_INTERVAL_MS = 16;  // ~60 fps
inline constexpr int POLL_INTERVAL_MS   = 5;   // how often the keyboard is checked
inline constexpr int DEFAULT_SPEED_MS   = 60;
inline constexpr int MIN_SPEED_MS       = 1;
inline constexpr int MAX_SPEED_MS       = 10000;
inline constexpr int CURSOR_BLINK_MS    = 500;

// layout
inline constexpr int BOX_HEIGHT      = 10;
inline constexpr int LOG_CAPACITY    = 200;
inline constexpr int MAX_INPUT_CHARS = 200;

inline constexpr const char* DEFAULT_TEXT = "hello world";
inline constexpr bool BOUNCE = true;  // false = scroll sideways instead

// colors, xterm 256-color codes (DLSU green)
inline constexpr int LOGO_GRADIENT[] = {157, 120, 84, 78, 41, 35};
inline constexpr int COLOR_INFO   = 151;
inline constexpr int COLOR_BORDER = 22;
inline constexpr int COLOR_ART    = 120;
inline constexpr int COLOR_ACCENT = 84;
inline constexpr int COLOR_DIM    = 65;
inline constexpr int COLOR_TEXT   = 194;
inline constexpr int COLOR_ERROR  = 203;
inline constexpr int COLOR_PILL   = 16;

// box drawing, swap for + - | if the terminal font can't show them
inline constexpr const char* BOX_TOP_LEFT     = "╭";
inline constexpr const char* BOX_TOP_RIGHT    = "╮";
inline constexpr const char* BOX_BOTTOM_LEFT  = "╰";
inline constexpr const char* BOX_BOTTOM_RIGHT = "╯";
inline constexpr const char* BOX_HORIZONTAL   = "─";
inline constexpr const char* BOX_VERTICAL     = "│";

}
