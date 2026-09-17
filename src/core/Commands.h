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

// help prints itself from this table, so it can't go stale
const std::vector<Command>& all();

void execute(Marquee& marquee, const std::string& line);

}
