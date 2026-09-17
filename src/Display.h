#pragma once

#include "Marquee.h"

namespace display {

// Turns on colors, hides the cursor, and routes Ctrl+C into `quit`.
void init(std::atomic<bool>& quit);
void restore();

// Redraws the screen ~60 times a second until quit. Runs on its own thread.
void renderLoop(Marquee& marquee);

}
