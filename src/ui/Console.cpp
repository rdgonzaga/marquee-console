#include "ui/Console.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

namespace console {

namespace {

std::atomic<bool>* quitFlag = nullptr;
DWORD originalMode = 0;
UINT originalCodePage = 0;

BOOL WINAPI onCtrlEvent(DWORD) {
    if (quitFlag) *quitFlag = true;
    return TRUE;
}

}

void init(std::atomic<bool>& quit) {
    quitFlag = &quit;
    SetConsoleCtrlHandler(onCtrlEvent, TRUE);

    // without this the default ~15ms timer caps us at about 33 fps
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

Size size() {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) return {120, 30};
    return {info.srWindow.Right - info.srWindow.Left + 1, info.srWindow.Bottom - info.srWindow.Top + 1};
}

void write(const std::string& text) {
    DWORD written = 0;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
}

std::string fg(int color) { return "\x1b[38;5;" + std::to_string(color) + "m"; }
std::string bg(int color) { return "\x1b[48;5;" + std::to_string(color) + "m"; }

}
