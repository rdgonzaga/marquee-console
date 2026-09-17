# CSOPESY MO3: Marquee Console

An OS emulator console with a command interpreter and an animated ASCII marquee.
The marquee keeps moving while you type.

## Members

- Gonzaga, Rainer
- Gonzales, Aaron James
- Ramos, Richmond Jose
- Yasumuro, Mariel

## Entry point

`src/main.cpp`

## Running

**Visual Studio 2022:** open `MarqueeConsole.sln`, pick Debug or Release / x64, press Run.

**Without Visual Studio:** run `build\Release\marquee.exe`. Everything is compiled
in, so the .exe works on its own from any folder.

## Commands

| Command | Description |
| --- | --- |
| `help` | display the commands and their descriptions |
| `start_marquee` | start the marquee animation |
| `stop_marquee` | stop the marquee animation |
| `set_text <text>` | display the given text as a marquee |
| `set_speed <ms>` | set the animation refresh in milliseconds (1-10000) |
| `exit` | terminate the console |

Extra keys: Up/Down for history, Tab to autocomplete, PgUp/PgDn to scroll output,
Esc to clear the line.

## How it works

Three threads share one `Marquee` struct, protected by a single mutex:

| Thread | Rate | Job |
| --- | --- | --- |
| Input (main) | every 5 ms | polls the keyboard without blocking, runs commands |
| Marquee | every `set_speed` ms | moves the art one step |
| Render | every 16 ms (~60 fps) | draws the whole screen in one write |

Refresh is deliberately separate from marquee speed, so typing stays instant even
at `set_speed 2000`. Each frame is built into one string and written in a single
call, which is what avoids flicker and tearing. `timeBeginPeriod(1)` is needed
because Windows' default ~15 ms timer would otherwise cap the refresh at ~33 fps.

## Files

```
src/main.cpp      starts the three threads
src/Config.h      refresh rate, polling rate, default speed, colors, box size
src/AsciiArt.*    the letter shapes, the logo and the group info (plain text)
src/Marquee.*     shared state and the bounce/scroll motion
src/Display.*     console setup and screen drawing
src/Commands.*    keyboard polling and the command interpreter
```

## Changing the ASCII art

All of it is plain text in `src/AsciiArt.cpp`, safe to paste over:

- `GLYPHS` is one row per character, starting at space. Keep a character's six
  rows the same width as each other.
- `LOGO` is the header art.
- `INFO` is the group names and version date.

Colors are in `src/Config.h` as xterm 256-color codes. `BOUNCE = false` there
switches the marquee from bouncing to scrolling sideways.
