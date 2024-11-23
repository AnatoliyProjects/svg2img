# svg2img v. 1.0 

**Header-only C++ library for converting SVG to raster images (PNG/JPEG/WebP) via browser.**

Required Emscripten/WebAssembly environment.

The MIT License (MIT). Copyright © 2024 Anatoly Petrov <petrov.projects@gmail.com>

# Rationale

Rasterization in the WebAssembly field may be tricky. In fact, it is.

The majority of C++ graphics libraries with SVG support have complicated
build pipelines. This complexity is caused by using of non-standard/non-widespread 
build systems (like Meson for cairo), many external dependencies, 
an extensive set of build flags, etc.

Furthermore, these libraries generally don't have native Emscripten/WebAssembly support, 
so if you want to use them in a browser, you should recreate the build pipeline yourself.

Finally, their API is overkill for converting a single SVG to PNG/JPEG/WebP 
(maybe with basic positioning/resizing).

What about alternatives? Take a look at svg2img!

# Design

In the Emscripten/WebAssembly world, we have full access to HTML/JS native features directly
from our C++ code. Specifically, we may programmatically create and use such HTML elements 
as `<img>` and `<canvas>`. These are all we need to rasterize our SVG. Just let the browser 
do its work.

Rasterization via a browser may look like this:

- Encode your input SVG as data URI;
- Provide this URI to the `src` attribute of `<img>` element;
- After `<img>` loaded, draw it on `<canvas>` via the Canvas API;
- You may also specify rescaling (without losing quality), positioning, etc.;
- Next, export rasterized data with `canvas.toBlob()` as PNG/JPEG/WebP;
- Congrats! You have your raster image.

svg2img does exactly that - with a friendly C++ API for the end user.

# Usage examples

## Basic case

You need to define a callback to receive a rasterized image and execute `raster::SvgToImage()` 
to do all the work.

Possible code:

```cpp
// example.cpp

// ...

#include <iostream>
#include <string>
#include <string_view>

#include "svg2img.h"

static const char* msg = "Hi! I'm just a metadata!";
static std::string png;

// It may also be a lambda, functor, or anything else convertible to the std::function<>.
void Cb(std::string_view img, raster::Error err, void * meta) {
    if (static_cast<bool>(err)) {
        std::cout << "Error occurs: " << raster::ToCStr(err) << std::endl;
        return;
    }
    // ok
    std::string img_fmt = raster::ToCStr(raster::GetImageFormat(img));
    std::cout << "Image format: " << img_fmt << '\n';
    std::cout << "Image header: " << raster::GetImageHeader(img) << '\n';
    std::cout << "Image size: " << img.size() << '\n';
    std::cout << "Your metadata: " << static_cast<char*>(meta) << std::endl;
    // Note that raster::SvgToImage() frees the image buffer after the callback returns.
    // Copy the image data if you want to use it further.
    png = img;
};

void ConvertToPng(std::string_view svg) {
    // You may choose the other output format ("image/jpeg" or "image/webp").
    // You may also specify the output image quality, image position on the canvas (x, y args),
    // and image size (width, height args).
    raster::SvgToImage(svg, Cb, const_cast<char*>(msg), "image/png");
}

// ...
```

Output example:

> Image format: png  
Image header: 89 50 4E 47 0D 0A 1A 0A  
Image size: 27035  
Your metadata: Hi! I'm just a metadata!

## Usage with Dear ImGui

GitHub: https://github.com/ocornut/imgui

Tech stack: Emscripten + Dear ImGui + GLFW + OpenGL + stb

--------

Dear ImGui is a bloat-free graphical user interface library for C++ with minimal dependencies.
It is widely used in game engines (for tooling), real-time 3D applications, fullscreen applications, 
embedded applications, etc.

Also, Dear ImGui is very friendly to Emscripten/WebAssembly. With Dear ImGui, you can run 
a full-featured UI in your browser with just ≈200 lines of code.

Dear ImGui supports raster images (prepared textures) with `ImGui::Image()`,
but SVG rasterization is reasonably beyond its domain's scope. 
So, if you need a simple tool to convert your SVG to PNG, please try svg2img.

You may find the Dear ImGui + svg2img example at `svg2img/example/main.cpp`.
This directory also contains Make/CMake files for building it.
See comments in the mentioned files for details.

You may also use the Img2Svg Demo as a playground to test the browser's (and svg2img's) 
limitations with different SVG samples/output options.

This is what Img2Svg Demo looks like:

![Img2Svg Demo](/assets/demo.png)

# Limitations

As with everything else, svg2img is not a silver bullet. Its performance depends on the browser/underlying JS 
and may not comply with your requirements. Also, you may feel that some meaningful features are lacking.
At this point, you probably should build cairo (or another graphics library of your choice) with Emscripten.

Finally, some browsers (including Safari) don't support WebP export and fall back to PNG. 
Also, all browsers strictly limit the maximum image size.

Anyway, if something goes wrong, svg2img gives you a descriptive error code and specifies the stage
at which the rasterization failed. See comments to `raster::Error` in `svg2img/include/svg2img.h` 
for details.

# Pitfalls

***First***, as mentioned above, svg2img has browser-specific limitations.
Familiarize yourself with them regarding your use case. Then, try the Img2Svg Demo 
with browsers and output options you should support.

***Second***, consider that `raster::SvgToImage()` is an asynchronous function, so you should write
your client code in a specific way. Pay particular attention to object lifetime management.

Yes, the library's design prevents the most common bugs in the field. For example, svg2img always 
creates a copy of your callback and format string - to guarantee that these objects 
will exist at the moment of usage (which will be _after_ the caller of `raster::SvgToImage()` returns 
and _after_ all temporary/local objects of the caller will be destroyed).

But, if your callback captures references to local/temporary objects of the caller, 
then, at the moment of callback execution, you will get dangling references.

***Third***, your callback should be noexcept and not fail in any other way. 
If your callback fails, svg2img does not guarantee that the allocated heap memory will be freed.

# Building

svg2img is a header-only library. You can include it in your project without special compile/link steps.

If you are new to Emscripten or Dear ImGui, you may find helpful 
the examples of Make/Cmake files placed in the `svg2img/example/` directory.

CLion users may also want to look at the `CMakeLists.txt` comment section, 
which describes the required steps for building Emscripten-powered applications from CLion.

# Credits

svg2img was inspired by helper libraries for Emscripten, published by 
[Armchair Software](https://github.com/Armchair-Software), and developed by https://github.com/slowriot.

# License

svg2img is licensed under the MIT License, see [LICENSE.txt](LICENSE.txt) for more information.
