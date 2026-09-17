#pragma once

#include <string>
#include <vector>

#include "core/Marquee.h"

namespace commands {

struct Command {
    const char* name;
    const char* usage;
    const char* description;
    void (*run)(Marquee& marquee, const std::string& args);
};

// The whole command set. `help` prints itself from this table.
const std::vector<Command>& all();

// Splits a typed line into command plus arguments and runs it.
void execute(Marquee& marquee, const std::string& line);

}
