#pragma once

#include "core/Marquee.h"

namespace input {

// Reads the keyboard and runs commands until quit. Runs on the main thread.
void loop(Marquee& marquee);

}
