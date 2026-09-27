#include "core/Marquee.h"

#include <algorithm>
#include <chrono>

#include "art/AsciiArt.h"

namespace {

void stepBounce(Marquee& m) {
    int maxX = std::max(0, m.boxWidth - m.artWidth);
    int maxY = std::max(0, m.boxHeight - static_cast<int>(m.art.size()));

    // the window may have shrunk since the last step
    m.x = std::clamp(m.x, 0, maxX);
    m.y = std::clamp(m.y, 0, maxY);

    if (maxX > 0) {
        if (m.x + m.dx < 0 || m.x + m.dx > maxX) m.dx = -m.dx;
        m.x += m.dx;
    }
    if (maxY > 0) {
        if (m.y + m.dy < 0 || m.y + m.dy > maxY) m.dy = -m.dy;
        m.y += m.dy;
    }
}

void stepScroll(Marquee& m) {
    m.y = std::max(0, m.boxHeight - static_cast<int>(m.art.size())) / 2;
    m.x -= 1;
    if (m.x < -m.artWidth) m.x = m.boxWidth;
}

}

void marqueeLoop(Marquee& m) {
    std::unique_lock<std::mutex> lock(m.mtx);
    while (!m.quit) {
        if (m.running) {
            if (config::BOUNCE) stepBounce(m);
            else stepScroll(m);
        }
        // drops the lock while asleep, and set_speed or exit wakes it early
        m.wake.wait_for(lock, std::chrono::milliseconds(m.speedMs));
    }
}

void setText(Marquee& m, const std::string& text) {
    m.text = text;
    m.art = ascii::render(text);
    m.artWidth = 0;
    for (const std::string& row : m.art) {
        m.artWidth = std::max(m.artWidth, static_cast<int>(row.size()));
    }
    m.x = 0;
    m.y = 0;
    m.dx = 1;
    m.dy = 1;
}

void addLog(Marquee& m, LogKind kind, const std::string& text) {
    m.log.push_back({kind, text});
    while (static_cast<int>(m.log.size()) > config::LOG_CAPACITY) m.log.pop_front();
    m.logScroll = 0;
}
