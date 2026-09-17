#include "Display.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

#include "AsciiArt.h"

namespace display {

namespace {

std::atomic<bool>* quitFlag = nullptr;
DWORD originalMode = 0;
UINT originalCodePage = 0;

BOOL WINAPI onCtrlEvent(DWORD) {
    if (quitFlag) *quitFlag = true;
    return TRUE;
}

void write(const std::string& text) {
    DWORD written = 0;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
}

struct Size {
    int width;
    int height;
};

Size consoleSize() {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) return {120, 30};
    return {info.srWindow.Right - info.srWindow.Left + 1, info.srWindow.Bottom - info.srWindow.Top + 1};
}

std::string fg(int color) { return "\x1b[38;5;" + std::to_string(color) + "m"; }
std::string bg(int color) { return "\x1b[48;5;" + std::to_string(color) + "m"; }
const char* const RESET = "\x1b[0m";
const char* const BOLD  = "\x1b[1m";

// One screen line, clipped to the window width. Color codes take no width.
class Line {
public:
    explicit Line(int maxWidth) : maxWidth_(maxWidth) {}

    Line& color(const std::string& code) {
        out_ += code;
        return *this;
    }

    Line& text(const std::string& s) {
        for (char c : s) {
            // UTF-8 continuation bytes don't start a new column
            if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
                if (width_ >= maxWidth_) break;
                ++width_;
            }
            out_ += c;
        }
        return *this;
    }

    Line& repeat(const std::string& s, int times) {
        for (int i = 0; i < times; ++i) text(s);
        return *this;
    }

    int width() const { return width_; }
    std::string str() const { return out_ + RESET; }

private:
    std::string out_;
    int width_ = 0;
    int maxWidth_;
};

void drawHeader(int width, std::vector<std::string>& lines) {
    static const std::vector<std::string> logo = ascii::split(ascii::LOGO);
    static const std::vector<std::string> info = ascii::split(ascii::INFO);
    const int shades = static_cast<int>(sizeof(config::LOGO_GRADIENT) / sizeof(config::LOGO_GRADIENT[0]));

    size_t logoWidth = 0;
    for (const std::string& row : logo) logoWidth = std::max(logoWidth, row.size());

    // logo on the left, group info on the right
    for (size_t r = 0; r < std::max(logo.size(), info.size()); ++r) {
        std::string logoRow = r < logo.size() ? logo[r] : "";
        logoRow.resize(logoWidth, ' ');
        int shade = logo.empty() ? 0 : std::min(shades - 1, static_cast<int>(r * shades / logo.size()));

        Line line(width);
        line.color(BOLD).color(fg(config::LOGO_GRADIENT[shade])).text(logoRow).color(RESET);
        if (r < info.size()) line.text("    ").color(fg(config::COLOR_INFO)).text(info[r]);
        lines.push_back(line.str());
    }
}

void drawBox(const Marquee& m, int width, std::vector<std::string>& lines) {
    int inner = m.boxWidth;

    std::string title = " \"" + m.text + "\" ";
    size_t maxTitle = static_cast<size_t>(std::max(5, inner / 2));
    if (title.size() > maxTitle) title = title.substr(0, maxTitle - 4) + "... ";

    Line top(width);
    top.color(fg(config::COLOR_BORDER)).text(config::BOX_TOP_LEFT).text(config::BOX_HORIZONTAL);
    top.color(fg(config::COLOR_ART)).text(title);
    top.color(fg(config::COLOR_BORDER)).repeat(config::BOX_HORIZONTAL, 1 + inner - top.width());
    top.text(config::BOX_TOP_RIGHT);
    lines.push_back(top.str());

    for (int row = 0; row < m.boxHeight; ++row) {
        std::string content(inner, ' ');
        int artRow = row - m.y;
        if (artRow >= 0 && artRow < static_cast<int>(m.art.size())) {
            const std::string& src = m.art[artRow];
            for (int col = 0; col < static_cast<int>(src.size()); ++col) {
                int screenCol = m.x + col;
                if (screenCol >= 0 && screenCol < inner) content[screenCol] = src[col];
            }
        }
        Line line(width);
        line.color(fg(config::COLOR_BORDER)).text(config::BOX_VERTICAL);
        line.color(BOLD).color(fg(config::COLOR_ART)).text(content).color(RESET);
        line.color(fg(config::COLOR_BORDER)).text(config::BOX_VERTICAL);
        lines.push_back(line.str());
    }

    Line bottom(width);
    bottom.color(fg(config::COLOR_BORDER)).text(config::BOX_BOTTOM_LEFT).repeat(config::BOX_HORIZONTAL, inner);
    bottom.text(config::BOX_BOTTOM_RIGHT);
    lines.push_back(bottom.str());
}

void drawStatus(const Marquee& m, int width, std::vector<std::string>& lines) {
    Line line(width);
    if (m.running) line.color(bg(config::COLOR_ACCENT)).color(fg(config::COLOR_PILL)).color(BOLD).text(" > RUNNING ");
    else           line.color(bg(config::COLOR_DIM)).color(fg(config::COLOR_PILL)).color(BOLD).text(" # STOPPED ");
    line.color(RESET);
    line.color(fg(config::COLOR_DIM)).text("  speed ").color(fg(config::COLOR_TEXT)).text(std::to_string(m.speedMs) + "ms");
    line.color(fg(config::COLOR_DIM)).text("  ·  " + std::to_string(m.fps) + " fps");
    if (m.logScroll > 0) line.color(fg(config::COLOR_ACCENT)).text("  ·  scrolled up (PgDn)");
    lines.push_back(line.str());
}

void drawLog(const Marquee& m, int width, int rows, std::vector<std::string>& lines) {
    int end = std::max(0, static_cast<int>(m.log.size()) - m.logScroll);
    int start = std::max(0, end - rows);
    for (int i = start; i < end; ++i) {
        Line line(width);
        switch (m.log[i].kind) {
            case LogKind::Command: line.color(fg(config::COLOR_ACCENT)).text(" > ").color(fg(config::COLOR_DIM)).text(m.log[i].text); break;
            case LogKind::Info:    line.color(fg(config::COLOR_TEXT)).text("   " + m.log[i].text); break;
            case LogKind::Error:   line.color(fg(config::COLOR_ERROR)).text(" ! " + m.log[i].text); break;
        }
        lines.push_back(line.str());
    }
    // blank padding keeps the prompt on the bottom row
    for (int i = end - start; i < rows; ++i) lines.push_back("");
}

std::string drawPrompt(const Marquee& m, int width, bool cursorOn) {
    const std::string label = "Command> ";
    int room = std::max(0, width - static_cast<int>(label.size()) - 1);
    std::string typed = m.input;
    if (static_cast<int>(typed.size()) > room) typed = typed.substr(typed.size() - room);

    Line line(width);
    line.color(BOLD).color(fg(config::COLOR_ACCENT)).text(label).color(RESET);
    line.color(fg(config::COLOR_TEXT)).text(typed);
    line.color(fg(config::COLOR_ACCENT)).text(cursorOn ? "_" : " ");
    return line.str();
}

std::string buildFrame(Marquee& m, int width, int height, bool cursorOn) {
    int usable = std::max(10, width - 1);  // writing the last column can wrap
    m.boxWidth = std::max(1, usable - 2);

    std::vector<std::string> top;
    drawHeader(usable, top);
    top.push_back("");
    drawBox(m, usable, top);
    drawStatus(m, usable, top);
    top.push_back("");

    // small window: drop header rows so the prompt stays visible
    int topRows = std::min(static_cast<int>(top.size()), std::max(0, height - 1));
    int logRows = std::max(0, height - topRows - 1);

    std::vector<std::string> lines(top.end() - topRows, top.end());
    drawLog(m, usable, logRows, lines);
    lines.push_back(drawPrompt(m, usable, cursorOn));

    // home the cursor, then erase to end of each line as we go
    std::string frame = "\x1b[H";
    for (size_t i = 0; i < lines.size(); ++i) {
        frame += lines[i] + "\x1b[K";
        if (i + 1 < lines.size()) frame += "\n";
    }
    return frame + "\x1b[J";
}

}

void init(std::atomic<bool>& quit) {
    quitFlag = &quit;
    SetConsoleCtrlHandler(onCtrlEvent, TRUE);

    // the default ~15ms timer would cap the frame rate at about 33 fps
    timeBeginPeriod(1);

    originalCodePage = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);

    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(out, &originalMode);
    SetConsoleMode(out, originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);

    // separate full-screen buffer, hidden cursor, cleared screen
    write("\x1b[?1049h\x1b[?25l\x1b[2J");
}

void restore() {
    write("\x1b[0m\x1b[?25h\x1b[?1049l");
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), originalMode);
    SetConsoleOutputCP(originalCodePage);
    timeEndPeriod(1);
}

void renderLoop(Marquee& m) {
    using clock = std::chrono::steady_clock;
    std::string lastFrame;
    Size lastSize{0, 0};
    int frames = 0;
    auto secondStart = clock::now();

    while (!m.quit) {
        auto frameStart = clock::now();
        Size size = consoleSize();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(frameStart.time_since_epoch()).count();
        bool cursorOn = (ms / config::CURSOR_BLINK_MS) % 2 == 0;

        std::string frame;
        {
            std::lock_guard<std::mutex> lock(m.mtx);
            frame = buildFrame(m, size.width, size.height, cursorOn);
        }

        if (size.width != lastSize.width || size.height != lastSize.height) {
            frame = "\x1b[2J" + frame;
            lastSize = size;
        }
        // one write per frame, and skip it entirely if nothing changed
        if (frame != lastFrame) {
            write(frame);
            lastFrame = std::move(frame);
        }

        ++frames;
        if (clock::now() - secondStart >= std::chrono::seconds(1)) {
            std::lock_guard<std::mutex> lock(m.mtx);
            m.fps = frames;
            frames = 0;
            secondStart = clock::now();
        }

        std::this_thread::sleep_until(frameStart + std::chrono::milliseconds(config::RENDER_INTERVAL_MS));
    }
}

}
