#pragma once

#include "core/Marquee.h"

namespace render {

// Redraws the screen ~60 times a second until quit. Runs on its own thread.
void loop(Marquee& marquee);

}
