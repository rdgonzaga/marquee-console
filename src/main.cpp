#include <thread>

#include "core/Commands.h"
#include "core/Marquee.h"
#include "ui/Console.h"
#include "ui/Input.h"
#include "ui/Render.h"

// Three threads run at once: this one reads the keyboard and runs commands,
// one moves the marquee, one redraws the screen. They share `marquee`,
// protected by the mutex inside it.
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

    // taking the lock once guarantees the marquee thread is waiting when notified
    marquee.quit = true;
    { std::lock_guard<std::mutex> lock(marquee.mtx); }
    marquee.wake.notify_all();
    marqueeThread.join();
    renderThread.join();

    console::restore();
    return 0;
}
