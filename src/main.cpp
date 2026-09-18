#include <thread>

#include "core/Commands.h"
#include "core/Marquee.h"
#include "ui/Console.h"
#include "ui/Input.h"
#include "ui/Render.h"

// Three threads: this one reads keys, one moves the marquee, one redraws.
// They all share the Marquee below, guarded by the mutex inside it.
int main() {
    Marquee marquee;

    console::init(marquee.quit);
    {
        std::lock_guard<std::mutex> lock(marquee.mtx);
        setText(marquee, config::DEFAULT_TEXT);
        addLog(marquee, LogKind::Info, "Type 'help' to see available commands.");
    }

    std::thread marqueeThread(marqueeLoop, std::ref(marquee));
    std::thread renderThread(render::loop, std::ref(marquee));

    input::loop(marquee);

    // grabbing the lock first means the marquee thread is parked on the wait
    marquee.quit = true;
    { std::lock_guard<std::mutex> lock(marquee.mtx); }
    marquee.wake.notify_all();
    marqueeThread.join();
    renderThread.join();

    console::restore();
    console::write("Terminating console...\n");
    return 0;
}
