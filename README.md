
# Login Retry

A *Spore* mini-mod for the login to the Spore servers. It fixes the game's HTTP reader, which takes some valid answers
of the server for a failed connection, and when a login fails for a reason that can pass by itself, it has the game
ask the server again a few times before it gives up and shows its message.

Current version: **1.0.0**. Download `LoginRetry.sporemod` from the [Login Retry 1.0.0](../../releases/tag/v1.0.0)
release.

## What it changes

When Spore starts with a saved user name and password, it logs in to the Spore servers by itself. The game sends that
request once. Sometimes it fails — it happens when the game is restarted often and quickly — and the game shows *"There was an error connecting to the Spore servers…"* and goes on offline. To get
online the player has to log in by hand.

### Answers without a reason phrase

An HTTP answer begins with a status line such as `HTTP/1.1 200 OK`. The text after the number, the reason phrase, may
be empty: `HTTP/1.1 200 ` is a valid status line, and servers do send it. The game's HTTP reader, however, takes any
status line shorter than 14 characters for an answer that is not HTTP, whatever the number says, and the game reports
it as a failed connection. Login Retry changes the one byte of the game's code that holds this length, so a status
line with an empty reason phrase is read like any other: a success logs the player in, and an error is reported as
the error it is.

The failed logins seen before the fix were of this family: the server answered as fast as it answers a login that
works, and the game could not read the answer's first line. What that line said was not captured; since the byte is
changed, the failure has not come back.

### A login that failed is tried again

A login that failed for one of these reasons is sent again after 1, 2, 4 and 8 seconds, while the game's *Connecting*
window stays on screen, for as long as the whole login has not taken more than 25 seconds:

- the server could not be reached: no connection, a connection that broke, no answer in time;
- the server answered with HTTP 5xx;
- the game's "internal application error".

The first try that works logs the player in as usual, and no message is shown. If every try fails, the game shows the
same message as without the mod and goes on offline, as before. A server name that cannot be found is the usual sign
of a computer that is offline, so that failure is tried only once more, a second later.

The same applies to a login from the game's Login window.

## What it does not change

- **A refused user name or password** (HTTP 401) is never sent again; neither is an account that is not validated, an
  account without the right to the product's online features, or a refused installation code. The game shows its
  message at once, as before.
- **An answer that turns the login away** (any other HTTP 4xx) and **an answer the game still cannot read** are not
  sent again either: when the game could not read the answer, four more tries within 21 seconds got the same one.
- **The login that links the installation to the account** ("Confirm Entitlement") is left alone.
- **Cancel** in the *Connecting* window works at any moment, also between two tries, and stops them.
- **Nothing happens in the background.** Once the game has shown its message, the mod does nothing more until the
  next login.
- **The saved login** (`login.prop`) is neither read nor written by the mod. It takes the user name and the password
  from the game's memory at the moment of a retry, hands them to the game's own login function and keeps no copy.
  The mod writes no files at all.

The changed byte belongs to the reader of every HTTP answer the game receives, not only of the login's. An answer
with a reason phrase is read exactly as before.

## Performance

The mod does its work only while the game logs in. During play it costs nothing.

## Installation

1. Install the [Spore ModAPI Launcher Kit](https://davoonline.com/sporemodder/rob55rod/ModAPI/Public/index.html)
   if you do not have it yet.
2. Open `LoginRetry.sporemod` with **Spore ModAPI Easy Installer**.
3. Start the game with **Spore ModAPI Launcher**. If the game was already running, restart it completely.

To remove the mod, use **Spore ModAPI Easy Uninstaller → Login Retry**. The mod stores nothing in saves.

Login Retry supports *Spore Galactic Adventures* with the March 2017 patch (`SporeApp_ModAPIFix.exe`, the version
the ModAPI Launcher Kit runs). On any other game build it changes nothing and shows a message instead. It also shows
a message when the game's login code is not the one it knows, and then leaves the login as it is.

## Other mods

Login Retry changes three entries in two of the game's function tables — the start of a login, the answer to a login
request and the result of a login — and one byte of the game's code, the shortest status line its HTTP reader takes.
When one of these entries, the code they lead to or the code around that byte is not what the game ships — another
mod has changed it — it leaves everything alone and says so in a message. It has run in the game together with
[Civ Challenge](https://github.com/codestlover/Spore-CivChallenge),
[Truely!NO-LOD:Enchanted](https://github.com/codestlover/Spore-TruelyNoLOD/tree/enchanted),
[Seamless Water](https://github.com/codestlover/Spore-SeamlessWater) and
[Beatuy!SpaceStage](https://github.com/codestlover/Spore-BeatuySpaceStage).

## How it works

The mod is a 32-bit DLL loaded by the ModAPI Launcher. After the game has finished starting up it:

- checks the game executable by its time stamp and image size, wherever Windows has placed it in memory;
- checks the bytes of the game's code at the eleven places it relies on and the five table entries that lead to five
  of the functions, and reads the place of the game's request queue out of the game's own accessor;
- puts its own functions into three table entries: the login manager's *log in* and *login result*, and the login
  request's *answer*;
- changes the length the HTTP reader asks of a status line from 14 to 13 characters, the length of
  `HTTP/1.1 200 ` with its empty reason phrase.

From then on, *log in* notes that a login has begun, *answer* notes what the network code reported, and *login
result* decides. A success, a failure that is not temporary, a result that did not come from a finished request, a
login that was not started through *log in*, a result on another thread than the game's, or a spent budget goes
straight to the game's own function. Otherwise the game's function is not called, so its *Connecting* window stays
open and no message appears, and a Windows timer on the game's thread is set. When it fires, the mod calls the same
game function the game's *log in* calls to queue a login request, with the user name and password the game holds at
that moment. If the game holds none any more, the last result goes to the game's own function instead.

The table entries and the byte are put back when the game shuts down.

## Building from source (Linux)

> **A note from the author:** I build and test this mod only on Linux, and I cannot help with a Windows build.
> To play the mod you don't need to build anything: just use the `.sporemod` from Releases.

The build is the same as for [Civ Challenge](https://github.com/codestlover/Spore-CivChallenge),
[Truely!NO-LOD](https://github.com/codestlover/Spore-TruelyNoLOD) and
[Seamless Water](https://github.com/codestlover/Spore-SeamlessWater), and the scripts in `cmake/` are shared with
them, which is why their settings start with `CIV_`. You need LLVM (`clang-cl`, `lld-link`, `llvm-lib`, `llvm-mt`,
`llvm-rc`), CMake 3.20 or newer, `make`, GNU `bash` 4.4 or newer, coreutils, findutils, grep with `-P`, `git`, `curl`,
`unzip`, `tar` and `jq`.

```sh
git clone https://github.com/codestlover/Spore-LoginRetry.git && cd Spore-LoginRetry
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-clangcl-i686.cmake \
    -DCIV_ACCEPT_MSVC_LICENSE=ON
cmake --build build
```

This produces `build/bin/LoginRetry.dll`. The first `cmake` run downloads the dependencies into `deps/` with
`cmake/setup.sh`; Login Retry itself needs `SporeModAPI.lib` (v2.5.559) and, with xwin, the MSVC CRT and Windows SDK
from them, which come under the Microsoft license and are not redistributed. `-DCIV_ACCEPT_MSVC_LICENSE=ON` means you
accept that license. If you already prepared these dependencies for one of the other mods, set `CIV_DEPS` to that
`deps` folder (an absolute path) before the first `cmake` run to reuse them.

The release was built with clang-cl 21.1.8 and CMake 4.3; built the same way, a fresh checkout of tag `v1.0.0` gives
byte for byte the DLL in the release.

Every text file starts with exactly one empty line and ends with exactly two; `cmake/layout.sh` checks this on each
build, and `bash cmake/layout.sh --fix` fixes it.

## Testing

The mod was tried in the game on 10 and 11 November 2026, in builds that also wrote a diagnostic log, since removed:

- with only the retries, one login out of twelve failed, on the third start of the game within six minutes. The mod
  sent it again four times from the game's own thread, every try was answered, and the game could read none of the
  answers, so it showed its message after the last one. The table entries, the timer and calling the game's login
  function again worked in the game;
- with the changed byte, all 30 logins worked at the first request, among them a third start within five minutes.

Releases are checked with tests that are not part of this repository:

- the game's own status line reader, taken from `SporeApp_ModAPIFix.exe`, runs in an emulator with the byte as
  shipped and as changed: as shipped it rejects `HTTP/1.1 200 `, changed it reads it, and every other line — with a
  reason phrase, too short, malformed, with a status outside 100–599 — is treated as before;
- a Wine test loads the built `LoginRetry.dll` into a stand-in for the game: a program with the game's image size
  and time stamp, placed at another base address than the game prefers, that holds at the game's own addresses
  functions beginning with the game's bytes, the two function tables in read-only memory, the code around the byte
  in memory that cannot be written, and the request queue. It plays logins against the DLL — one that works, server
  errors followed by a success, a server that stays away, a server name that is not found, a refused password, an
  account that is not validated, a missing entitlement, HTTP 429, an answer that is not HTTP, Cancel between two
  tries, the login that links the installation code, a password that is gone at the moment of the retry, a result on
  another thread, a new login between two tries — and checks every request the DLL sends, when it sends it, what the
  game is told, and that shutting down puts the tables and the byte back;
- the same program checks that the DLL refuses a table another mod changed, changed code and a foreign executable,
  each with its message and without changing anything;
- a second program reads `SporeApp_ModAPIFix.exe` itself and checks every byte and table entry the DLL relies on,
  and that the three functions it replaces are reached through their one table entry only.

## Credits and license

Login Retry is licensed under **GPL-3.0-or-later** (see [`LICENSE`](LICENSE)).

Full notices are in [`NOTICE.md`](NOTICE.md). Spore and Galactic Adventures belong to Electronic Arts/Maxis. The
repository and the release contain no game executables, assets or save data.

