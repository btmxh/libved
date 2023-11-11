# libved

Minimalist Video Editor

The idea here is that you use code to edit your video, but unlike existing solutions,
like [Remotion](https://www.remotion.dev) or [Manim](https://www.manim.community),
**libved** focuses on creating frame-perfect and complex edits, utilizing the full
potential of the GPU.

Currently, this video only supports X11 Linux systems with VAAPI. There are no plans
to extend this to other platforms, since only I use this thing and I don't want to
consider the use cases of imaginary people.

This editor is inspired by [kdenlive](https://kdenlive.org/en/)'s choopy playback,
[olive](https://www.olivevideoeditor.org)'s node system and the aforementioned
programmatically video editors.

## Roadmap

This editor aims to become somewhat like a server (because muh Unix philosophy), but
with a preview display (when editing). There are two ways to interface with this
server. The first way is to write scripts, and this is what one would primarily do
when editing videos. Another way is to use external programs to interface with this
thing via sockets or some networking thing (I haven't planned yet).

For example, if one want a customly made smooth transition between two scene, they
could "simply" write some OpenGL code to create that. On the other hand, if the
requirement is to sync the beat drop to that transition or something, manually
typing the timestamp would be torturing yourself, so use external programs for that.

Currently, I think that the external programs could simply generate scripts, so
there is effectively just one way to interface with this thing. I may consider other
options later.

Here is the roadmap:
- [x] Implement (hardware-accelerated) video decoding API (in C++).
- [ ] Implement (hardware-accelerated) audio decoding API (in C++).
- [ ] Sync video and audio and add decoding threads (in C++).
- [ ] Make Lua scripting interface, which exposes basically everything: OpenGL,
  FFmpeg, etc.
- [ ] Implement (hardware-accelerated) encoding API (in C++).
- [ ] Write (non-C++) external programs.
- [ ] Make the 2022-2023 Culture Rewind using this tool (couldn't be made public,
  sorry).

## Building

This requires a cutting-edge C++ compiler (supports C++20 and C11). Of course the
compiler vendors are not done with implementing everything, so I will only use what
my compiler (and the LSP) supports.

Dependencies (install them using your package manager):
- sol2: Lua wrapper
- FFmpeg, X11, GLFW, OpenGL, OpenAL: Self-explanatory
- fmt, tl-optional, cppcoro: things should've been in the C++ stdlib

Submodule dependencies:
- glad: OpenGL function pointer generator thingy
- vkfw (and GLFW): Wrapper for GLFW.

Bundled dependencies:
- staplegl: OpenGL wrapper.

To build, run:
```sh
# configure
cmake -S. -Bbuild
# build
cmake --build build
# run
./build/libved
```

