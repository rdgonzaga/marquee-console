#pragma once

#include <string>
#include <vector>

#include "Marquee.h"

namespace commands {

struct Command {
    const char* name;
    const char* usage;
    const char* description;
    void (*run)(Marquee& marquee, const std::string& args);
};

const std::vector<Command>& all();

// Reads the keyboard and runs commands until quit. Runs on the main thread.
void inputLoop(Marquee& marquee);

void execute(Marquee& marquee, const std::string& line);

}
