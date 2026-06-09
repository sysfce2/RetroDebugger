# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Git & GitHub hygiene

**Never push AI-coding artifacts to GitHub.** Implementation plans, design specs, and any
AI-assistant working files — e.g. `docs/superpowers/` (plans/specs), `.claude/`,
`.superpowers/`, `claude/` — must NOT be committed to or pushed to the remote repository.
They are gitignored; keep it that way. Do not `git add` them and do not push commits that
contain them. `CLAUDE.md` / `AGENTS.md` (this guidance file) is the only AI-related file
intentionally tracked.

**Never push to the remote without explicit user consent.** Commit locally, then ask before
pushing.

## Project Overview

Retro Debugger is a real-time debugger for 8-bit computers: Commodore 64, Atari XL/XE, and NES. It embeds full emulator engines (VICE v3.10-WIP, Atari800, NestopiaUE) and provides cycle-accurate debugging through an ImGui-based interface. Previously known as "C64 65XE NES Debugger".

## Logging

To enable `LOGD()`/`LOGM()` output, ensure `#define GLOBAL_DEBUG_OFF` is **commented out** in `MTEngineSDL/platform/MacOS/src.MacOS/DBG_Log.h` (line 33). When this define is active, all log macros compile to no-ops. By default logs are written to `~/Library/Caches/RetroDebugger-*.txt`. Use `--log-dir /tmp` to redirect them (the `tests/run_test.sh` script already does this).

## Build Commands

### macOS (Xcode)
```bash
xcodebuild -project ./platform/MacOS/c64d.xcodeproj -target "Retro Debugger"
```
Or open `platform/MacOS/c64d.xcodeproj` in Xcode.

### Linux (CMake)
```bash
./build-linux.sh        # Full build: clones MTEngineSDL + uSockets, builds everything
# Or manually:
mkdir -p build && cd build && cmake ../ && make -j$(nproc) retrodebugger
```

### Windows
Open `platform/Windows/c64d.sln` in Visual Studio 2019/2022.

### Keeping Build Projects in Sync
**IMPORTANT:** When adding, removing, or renaming source files in the Xcode project (`platform/MacOS/c64d.xcodeproj`), you MUST also update the Linux CMake (`CMakeLists.txt`) and Windows Visual Studio (`platform/Windows/c64d/c64d.vcxproj` + `.vcxproj.filters`) projects to match. All three build systems use explicit file lists — there is no auto-discovery. The same applies to MTEngineSDL — its Xcode, CMake, and Visual Studio projects must all be kept in sync when files change.

### Critical Dependency
**MTEngineSDL** must exist at `../../MTEngineSDL` relative to this repo. It provides SDL2 + ImGui integration, the GUI framework (`CGuiView`, `guiMain`), and all platform abstractions. Repo: https://github.com/slajerek/MTEngineSDL

**IMPORTANT: Do NOT modify MTEngineSDL directly.** It is an external library. If new functionality is needed in MTEngineSDL, flag this to the user and wait for permission. We will switch to the MTEngineSDL project space to implement changes there. Never implement or change anything in MTEngineSDL without explicit user approval.

**IMPORTANT: Do NOT create git worktree it is not supported in both c64d and MTEngineSDL repos.

## Architecture

### Emulator Abstraction Layer
`CDebugInterface` (in `src/DebugInterface/`) is the abstract base class all emulators implement. Concrete implementations:
- `CDebugInterfaceVice` (C64, inherits `CDebugInterfaceC64`) in `src/Emulators/vice/ViceInterface/`
- `CDebugInterfaceAtari` in `src/Emulators/atari800/AtariInterface/`
- `CDebugInterfaceNes` in `src/Emulators/nestopiaue/NestopiaInterface/`

Emulators are enabled/disabled via `#define` flags in `src/Emulators/EmulatorsConfig.h` (`RUN_COMMODORE64`, `RUN_ATARI`, `RUN_NES`).

### Data Adapter Pattern
`CDebugDataAdapter` provides uniform memory access across different address spaces (C64 RAM, cartridge, REU, 1541 drive RAM, NES PPU/OAM, Atari regions). All generic memory views (hex dump, data map, watches) work through this abstraction.

### View System
Views inherit from `CGuiView` (MTEngineSDL base class) and render via `RenderImGui()`. Key views are in `src/Views/` with platform-specific subfolders (`C64/`, `Atari800/`, `Nes/`).

### Central Coordinator
`CViewC64` (`src/Screens/CViewC64.cpp`) is the main application view: manages emulator instances, layout switching, and coordinates multi-threaded emulation. Despite the name, it manages all emulated platforms.

### App Lifecycle
Entry point is `src/RetroDebuggerAppInit.cpp`. MTEngineSDL calls `MT_PreInit()` -> `MT_PostInit()` (creates `CViewC64`). Settings folder: "RetroDebugger".

### Plugin System
Plugins extend `CDebuggerEmulatorPlugin` and hook into frame rendering and input. Registered in `src/Plugins/C64D_InitPlugins.cpp`. Examples: GoatTracker, CRT maker.

### Symbol & Breakpoint System
`CDebugSymbols` manages labels, breakpoints, and watches organized by `CDebugSymbolsSegment`. Supports VICE, KickAss, and other symbol file formats. Serialized as HJSON.

### Task System
`CDebugInterfaceTask` handles deferred/thread-safe emulator state changes, including VSync-synchronized operations.

### Remote Debugging
WebSocket-based server (`src/Remote/`) with JSON command protocol. Test client in `tools/websockets-debugger-test/`.

### Menu Bar
`CMainMenuBar` (`src/Views/CMainMenuBar.cpp`) is the largest single file (~4700 lines) containing all menu definitions and settings UI.

## Code Conventions

- Class names use `C` prefix (e.g., `CViewC64`, `CDebugInterface`)
- Logging via `LOGD()`, `LOGM()` macros from MTEngineSDL
- Configuration uses HJSON format (`CConfigStorageHjson`)
- Version string in `src/C64D_Version.h`
- Command-line parsing in `src/Tools/C64CommandLine.cpp`
- Settings storage in `src/Tools/C64SettingsStorage.cpp`

## GT2 Renoise Shortcuts

When adding a new functional key shortcut for the GoatTracker 2 Renoise layout,
update all three places together: the shortcut dispatcher
(`CGT2RenoiseInput`/focused GT2 views), the automated GT2 shortcut tests, and the
GoatTracker plugin menu (`C64DebuggerPluginGoatTracker::RenderMainMenuImGui()`).
If the shortcut is a user-visible command, add or update its menu item/shortcut
hint in the GT2 menu. Do not add GoatTracker-specific shortcut logic to
`CMainMenuBar`; the main menu should keep routing through generic plugin hooks.
Also keep `claude/architecture/gt2-tracks-and-channels.md` in sync with the
shortcut behavior and any main-row/numpad constraints.

## Writing Conventions

- **Never use `§` as a section marker.** Use `#` with the section number, e.g.
  `#11a.4`, `#15.4.1`. This applies to all prose: specs, docs, commit
  messages, PR bodies, code comments, and chat responses. The reason is
  plain-ASCII portability — `§` renders inconsistently across terminals,
  grep patterns, and diff tools, and the repo's existing spec/doc style
  already uses `#N` cross-references throughout (see
  `src/Plugins/Remapper/spec/remapper-6502-blitter.md` for the reference
  style).

## VICE 3.10 Upgrade (In Progress)

The embedded VICE engine is being upgraded from 3.1 to 3.10 on branch `upgrade/vice-execute`. **WARNING:** VICE 3.1 (OLD) and VICE 3.10 (NEW) are different releases years apart — do NOT confuse them. Reference sources: `vice-3.1-compiled/` (old), `vice-3.10/` (new target).

**Status & plan:** `claude/vice-310-upgrade-status.md` — contains complete summary of what's done, what remains, prioritized plan, blocking dependencies, and intentional divergences. **Always read this file before doing any VICE upgrade work.**

**VICIISC detailed plan:** `claude/README-VICIISC-UPGRADE.md`

Core emulation engine is upgraded (CPU, memory, VIC-II, CIA, VIA, SID, drive, interrupts, alarms, snapshots). Type migration (BYTE→uint8_t etc.) is complete. Remaining work: viciisc directory alignment, video/raster pipeline, new cart types, peripheral device registration, vdrive/fsdevice. 19/19 tests passing.

## Claude Workspace (`claude/`)

The `claude/` directory is Claude's dedicated workspace. All Claude-generated artifacts go here — **never** in project directories like `tools/`, `src/`, etc. (unless it's actual project source code).

- **`claude/tools/`** — Scripts and utilities created by Claude (e.g., migration helpers, code generators)
- **`claude/architecture/`** — Technical documentation about the codebase
- **`claude/`** (root) — Status files, upgrade plans, assessments

The `tools/` directory at the project root is reserved for user's project tools only (e.g., `Exomizer-Decrunch`, `c64d-champ`, `websockets-debugger-test`).

**Technical docs**: When implementing features, fixing bugs, or changing architecture, always update the relevant documentation in `claude/`. If a doc doesn't exist yet for the area you're changing, create one. Keep docs accurate and in sync with the code — outdated docs are worse than no docs. Key docs: `claude/architecture/testing.md` (test frameworks and CLI runner), `claude/external/MTEngineSDL.md` (engine API reference), `claude/vice-310-upgrade-status.md` (VICE upgrade status and plan).

## Git Commits

- Do NOT add "Co-Authored-By" lines to commit messages.
- **"Commit and push" means**: Before committing, review and update all relevant documentation in `claude/architecture/`. Then commit and push **both** RetroDebugger and MTEngineSDL (if MTEngineSDL has changes). Always check both repos.

## Testing

Dual-framework test system in `src/Tests/` (ported from LightHeroes):

1. **CTest/CTestSuite** — Async integration tests (emulator state, memory ops, breakpoints)
2. **imgui_test_engine** — UI automation tests (menu verification, view interaction)

Both run headlessly from CLI. See `claude/architecture/testing.md` for full details.

### Running Tests

```bash
# Shell script (builds + runs + parses results)
tests/run_test.sh                           # All suite tests
tests/run_test.sh EmulatorStartup           # Single test
tests/run_test.sh --skip-build EmulatorStartup  # Skip build

# Direct binary flags (always use --log-dir /tmp to avoid log files in ~/Library/Caches)
./retrodebugger --headless --log-dir /tmp --run-suite --exit-after-tests      # CTestSuite
./retrodebugger --headless --log-dir /tmp --run-tests --exit-after-tests      # ImGui tests
./retrodebugger --headless --log-dir /tmp --run-test EmulatorStartup --exit-after-tests  # Single test
```

### Visual / VIC Output Validation (Remapper plugin tests)

For tests that exercise on-screen rendering paths (Remapper plugin
blitter, sprite multiplexer, demo effects), RAM-level parity is
necessary but **NOT sufficient**. A test does NOT pass if it does
not also pass scoring of a VICE emulator screenshot. The same
generated bytes can render to different pixels depending on VIC
config, screen RAM, color RAM, sprite pointers, and multiplexer
state — none of which RAM byte-comparison catches.

Such tests must:

1. Run via the **prod path** — the same code path the user hits when
   they click the Generate button (`pluginRemapper->GeneratePRG()`).
   No test-only shortcuts. Env vars are allowed only for internal
   test-config tweaks (pattern selection, output paths), never for
   bypassing prod behavior.
2. Hook a CPU breakpoint at the "init done" address
   (`lastGeneratedAddrRepeat`), let the demo reach steady state,
   then frame-step ~25 frames via the VICE frame-step API.
3. Capture the C64 interior screen via `api->GetScreenImageWithoutBorders()`.
4. Sample expected pixel positions against the C64 palette and fail
   the test if rendered colors don't match the framebuffer pattern.
5. Always write `/tmp/<testname>-screen.png` and
   `/tmp/<testname>-state.txt` (VIC regs, screen/color RAM, sprite
   ptrs, mismatch list) regardless of pass/fail, for forensic inspection.

See `src/Plugins/Remapper/spec/blitter-screenshot-test-prompt.md`
for the canonical implementation prompt.

### Adding Tests

1. Create `src/Tests/CTestMyFeature.h/.cpp` inheriting from `CTest`
2. Register in `CTestSuite::RegisterAllTests()` (`src/Tests/CTestSuite.cpp`)
3. For UI tests, add to `RegisterRetroDebuggerTests()` in `src/Tests/CImGuiTests.cpp`

**Important:** Emulators (C64, Atari, NES) can be enabled/disabled via the File menu. Not all emulators may be running at test time. Tests that need a specific emulator must check `di->isRunning` and call `viewC64->StartEmulationThread(di)` + `SYS_Sleep(2000)` if not running, then restore the original state with `viewC64->StopEmulationThread(di)` afterward. See `CTestOpenAllViews` and `CTestStackAnnotation` for examples.

Manual testing with test assets in `assets/tests/` and PRG files is also supported.

## MCP Skill
When the retrodebugger MCP server is connected (any `mcp__retrodebugger*` tool is available),
you MUST READ `claude/mcp-server/skills/retrodebugger-mcp-skill.md` before using any MCP tools.
This file contains tool usage guidelines, safe debugging defaults, and workflows — including
the correct tool to use for each task (e.g. `retro_memory_search` for finding game state
variables, NOT manual memory reads).
