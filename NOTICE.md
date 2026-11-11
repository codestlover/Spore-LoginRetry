
# Notices

Login Retry 1.0.0 is licensed under GPL-3.0-or-later; see `LICENSE`.

Login Retry contains no code or assets of *Spore* or *Spore Galactic Adventures* by Maxis/Electronic Arts. To
recognise the game it works with, `src/core/Sites.hpp` holds the addresses of the game's functions and function
tables it uses and up to 20 bytes of the game's code at each of eleven places. The game's code remains the property
of its rights holders; the license above covers this project.

The game's login code was read with [Ghidra](https://ghidra-sre.org) and the
[Spore ModAPI SDK](https://github.com/Spore-Community/Spore-ModAPI), maintained by Eric Mor and the Spore community.
Login Retry links `SporeModAPI.lib` of the Spore ModAPI SDK, GPL-3.0-or-later, to have the installed
`SporeModAPI.dll` run its start and shutdown functions.

The MSVC CRT and the Windows SDK are build dependencies under Microsoft's licenses and are not redistributed.

