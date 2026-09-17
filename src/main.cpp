#include <thread>

#include "Commands.h"
#include "Display.h"
#include "Marquee.h"

// Three threads run at once: this one reads the keyboard and runs commands,
// one moves the marquee, one redraws the screen. They share `marquee`,
// protected by the mutex inside it.
int main() {
    Marquee marquee;

    display::init(marquee.quit);
    {
        std::lock_guard<std::mutex> lock(marquee.mtx);
        setText(marquee, config::DEFAULT_TEXT);
        addLog(marquee, LogKind::Info, "Type 'help' to see available commands.");
    }

    std::thread marqueeThread(marqueeLoop, std::ref(marquee));
    std::thread renderThread(display::renderLoop, std::ref(marquee));

    commands::inputLoop(marquee);

    // taking the lock once guarantees the marquee thread is waiting when notified
    marquee.quit = true;
    { std::lock_guard<std::mutex> lock(marquee.mtx); }
    marquee.wake.notify_all();
    marqueeThread.join();
    renderThread.join();

    display::restore();
    return 0;
}
