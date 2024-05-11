# sim

> [!WARNING]
> Work in progress

***s***_okol_ ***i***_mmediate_ ***m***_ode_ is a simple fixed-pipeline (legacy OpenGL) layer built on [sokol](https://github.com/floooh/sokol/) with built in batching. Inspired by [rlgl.h](https://github.com/raysan5/raylib/blob/master/src/rlgl.h) and [sokol_gl.h](https://github.com/floooh/sokol/blob/master/util/sokol_gl.h). Designed to be _simple_ to use and bind to other languages.

## TODO

- [X] ~~Built in automatic batched rendering~~
- [X] Textures
    - [X] ~~Loading~~ (stb_image.h+qoi.h)
    - [ ] Rendering
- [ ] Framebuffers
- [ ] Point size setting
- [ ] Gamepad support ([libstem_gamepad](https://github.com/ThemsAllTook/libstem_gamepad))
- [ ] Optional lighting
- [ ] Optional fog
- [ ] Improve error handling/reporting
- [ ] Fixed+variable framerate setting
- [ ] Clipboard functions

## Dependencies

- [floooh/sokol](https://github.com/floooh/sokol/) (zlib/libpng)
    - sokol_gfx.h
    - sokol_app.h
    - sokol_glue.h
    - sokol_time.h
    - sokol_log.h
- [HandmadeMath/HandmadeMath](https://github.com/HandmadeMath/HandmadeMath/) (CC0-1.0)
    - HandmadeMath.h
- [nothings/stb](https://github.com/nothings/stb/) (MIT/Unlicense)
    - stb_image.h
- [phoboslab/qoi](https://github.com/phoboslab/qoi/) (MIT)
    - qoi.h

## LICENSE
```
The MIT License (MIT)

Copyright (c) 2024 George Watson

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
