// svg2img v. 1.0
// Header-only library for converting svg to raster images (png/jpeg/webp) via browser.
// Required Emscripten/WebAssembly environment.
// SPDX-FileCopyrightText: Copyright © 2024 Anatoly Petrov <petrov.projects@gmail.com>
// SPDX-License-Identifier: MIT

// Header with all the content.

#ifndef EMSCRIPTEN_SVG2IMG_H
#define EMSCRIPTEN_SVG2IMG_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <format>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>

#include <emscripten/em_macros.h>

// ============================================================================
// End-user api
// ============================================================================

namespace raster {
    // Constants

    // Possible rasterization errors.
    // Error scheme reflects the stages of the svg rasterization via browser:
    // encoding svg as data uri -> loading data uri to <img> -> drawing <img> on <canvas> ->
    // extracting image blob from <canvas>.
    // Thus, it is highly likely that UriEncodingFailed/ImgLoadingFailed was thrown because of the
    // broken svg; CanvasDrawingFailed/BlobExportFailed - because the image parameters
    // are invalid or unsupported by the browser (for example, the output size is too large).
    enum class Error: int {
        None = 0, // Svg successfully rasterized.
        NoInputData, // Missed input data.
        UriEncodingFailed, // Unable to encode svg as data uri.
        ImgLoadingFailed, // Unable to load svg to <img>.
        CanvasDrawingFailed, // Unable to draw an image on <canvas>.
        BlobExportFailed, // Unable to extract blob from <canvas>.
    };

    // Possible raster formats.
    // The raster::SvgToImage() function returns a png image if the user-specified format
    // is not supported by the browser. Thus, the end user needs to have an api to easily
    // deduce the resulting format. This api includes the below enumeration and the following
    // helper functions: raster::GetImageHeader(), raster::GetImageFormat().
    enum class Format: int {
        Png = 0,
        Jpeg,
        Webp,
        Unknown,
    };

    // Core

    // Client's callback type.
    // The callback should receive a raster image, error flag, and pointer to the metadata.
    // It also should be noexcept. If the callback throws an error or fails in another way,
    // raster::SvgToImage() shows an alert message in the browser.
    using Callback = std::function<void(std::string_view img, Error err, void *meta)>;

    // Converts svg to raster image via the browser (C++ facade).
    // A client may specify metadata for the callback, output image format
    // ("image/png", "image/jpeg", "image/webp" - support depends on the browser),
    // output image quality (0.0f..1.0f), place of the output on the canvas (x, y),
    // and the output image size (width, height - if changing, set both values).
    // Note that the output image buffer is deallocated after the callback returns.
    // Thus, you should copy the image data to use it further.
    inline void SvgToImage(std::string_view svg, Callback cb, void *meta = nullptr,
                           const std::string& format = "image/png", float quality = 1.0f,
                           float x = 0.0f, float y = 0.0f, float width = 0.0f, float height = 0.0f);

    // Helpers

    // Returns the C-string representation of an error code.
    inline const char *ToCStr(Error err);

    // Returns the C-string representation of an image format.
    inline const char *ToCStr(Format fmt);

    // Returns image header as a hex substring.
    // Pos argument specifies header start, n - header length.
    // Output formatting example: "89 50 4E 47 0D 0A 1A 0A" (png header).
    inline std::string GetImageHeader(std::string_view img, std::size_t pos = 0,
                                      std::size_t n = 8);

    // Deduces the image format by the header.
    inline Format GetImageFormat(std::string_view img);
}

// ============================================================================
// Implementation details
// ============================================================================

// The standard EM_JS macro doesn't inline the function definition.
// Thus, if we include the current header in multiple translation units, we'll get
// the duplicate symbol error for __em_js__SvgToImage and __em_js_ref_SvgToImage.
// To avoid this, we add the inline keyword into the original __em_js signature
// and remove __em_js_ref.
// The original author of this approach: https://github.com/slowriot
// Usage example: https://github.com/Armchair-Software/emscripten-browser-file/blob/main/emscripten_browser_file.h
// EM_JS source: https://github.com/emscripten-core/emscripten/blob/main/system/include/emscripten/em_js.h
#define _EM_JS_INLINE(ret, c_name, js_name, params, code)                      \
  _EM_BEGIN_CDECL                                                              \
  ret c_name params EM_IMPORT(js_name);                                        \
  __attribute__((visibility("hidden")))                                        \
  EMSCRIPTEN_KEEPALIVE                                                         \
  __attribute__((section("em_js"), aligned(1))) inline char __em_js__##js_name[] = \
  #params "<::>" code;                                                         \
  _EM_END_CDECL

#define EM_JS_INLINE(ret, name, params, ...) _EM_JS_INLINE(ret, name, name, params, #__VA_ARGS__)

namespace raster::aux {
    // Pointer to the callback's copy.
    // To avoid a dangling pointer, we should guarantee that the callback object
    // remains until the aux::ExecCb() call. To achieve that, we copy the callback
    // to the dynamic memory in raster::SvgToImage() and free it in aux::ExecCb().
    using PCallback = const Callback * const;

    // Converts svg to raster image via the browser (JS implementation).
    EM_JS_INLINE(void, SvgToImage, (const char* data, std::size_t size, PCallback pcb,
                                    void* meta, const char* format, float quality,
                                    float x, float y, float width, float height), {
        // -------------------------------------------------------------------
        // Constants
        // -------------------------------------------------------------------

        const RasterError = {
            None: 0,
            NoInputData: 1,
            UriEncodingFailed: 2,
            ImgLoadingFailed: 3,
            CanvasDrawingFailed: 4,
            BlobExportFailed: 5,
        };

        // -------------------------------------------------------------------
        // Helpers
        // -------------------------------------------------------------------

        // Signals the client about an error.
        function failed(err) {
            Module.ccall("ExecCb",
                         "v", ["number", "number", "number", "number", "number"],
                         [pcb, 0, 0, err, meta]);
        }

        // Encodes row svg as data uri.
        // Messages the client about an error and returns null if encoding failed.
        function svgToDataUri(data, size) {
            const svg = UTF8ToString(data, size); // emsc
            try {
                return "data:image/svg+xml;charset=utf8," + encodeURIComponent(svg);
            } catch (e) {
                failed(RasterError.UriEncodingFailed);
                return null;
            }
        }

        // Draws svg on <canvas> for further image export.
        function drawSvg(img, fmt) {
            let canvas = document.createElement("canvas");
            // We should explicitly set <canvas> width/height.
            // Otherwise, default values will be applied (w=300, h=150).
            // https://developer.mozilla.org/en-US/docs/Web/HTML/Element/canvas
            setCanvasSize(canvas, width, height, img);
            let context = canvas.getContext("2d");
            try {
                if (width && height) { context.drawImage(img, x, y, width, height); }
                else { context.drawImage(img, x, y); }
                canvas.toBlob(processBlob, fmt, quality);
            } catch (e) { failed(RasterError.CanvasDrawingFailed); }
        }

        // Sets <canvas> width/height considering the input arguments or image size.
        function setCanvasSize(canvas, w, h, img) {
            if (width && height) {
                canvas.width = w;
                canvas.height = h;
            } else {
                canvas.width = img.width;
                canvas.height = img.height;
            }
        }

        // Processes the <canvas> blob.
        function processBlob(blob) {
            if (!blob) {
                failed(RasterError.BlobExportFailed);
                return;
            }
            function onRejected(e) { failed(RasterError.BlobExportFailed); }
            blob.arrayBuffer().then(loadImage, onRejected);
        }

        // Loads raster image on the heap and calls the callback.
        function loadImage(buf) {
            const arr = new Uint8Array(buf);
            const on_heap = Module._malloc(arr.length);
            writeArrayToMemory(arr, on_heap); // emsc
            try {
                Module.ccall("ExecCb",
                             "v", ["number", "number", "number", "number", "number"],
                             [pcb, on_heap, arr.length, RasterError.None, meta]);
            } catch(e) {
                // The callback function seems broken, so just alert.
                alert("svg2img: Callback error: " + e.toString());
            } finally {
                Module._free(on_heap);
            }
        }

        // -------------------------------------------------------------------
        // Main
        // -------------------------------------------------------------------

        const data_uri = svgToDataUri(data, size);
        if (data_uri === null) { return; }
        // 'format' may point to a temporary object. In this case, when the img.onload
        // event occurs, this object will be destroyed, and we will get the dangling pointer.
        // To prevent this, we are currently casting it to the JS string.
        const sformat = UTF8ToString(format);
        let img = document.createElement("img");
        img.src = data_uri;
        // For browser compatibility, we use two possible names of the same event.
        img.error = (event) => { failed(RasterError.ImgLoadingFailed); };
        img.onerror = (event) => { failed(RasterError.ImgLoadingFailed); };
        img.onload = (event) => { drawSvg(img, sformat); };
    });

    // Executes the client's callback.
    // This function suits the call from JS.
    extern "C"
    inline void EMSCRIPTEN_KEEPALIVE ExecCb(PCallback pcb,
                                            const char *data, std::size_t size,
                                            const Error err, void *meta) {
        const Callback cb = *pcb;
        if (data != nullptr and size > 0) cb({data, size}, err, meta);
        else cb(std::string_view(), err, meta);
        delete pcb;
    }
}

namespace raster {
    inline void SvgToImage(const std::string_view svg, Callback cb, void *meta,
                           const std::string& format, const float quality,
                           const float x, const float y,
                           const float width, const float height) {
        if (svg.empty() or svg[0] == '\0') {
            cb(std::string_view(), Error::NoInputData, meta);
            return;
        }
        // svg not empty
        aux::PCallback pcb = new Callback(std::move(cb));
        aux::SvgToImage(svg.data(), svg.size(), pcb, meta, format.c_str(),
                        quality, x, y, width, height);
    }

    inline const char *ToCStr(const Error err) {
        switch (err) {
            case Error::None: return "raster::Error::None";
            case Error::NoInputData: return "raster::Error::NoInputData";
            case Error::UriEncodingFailed: return "raster::Error::UriEncodingFailed";
            case Error::ImgLoadingFailed: return "raster::Error::ImgLoadingFailed";
            case Error::CanvasDrawingFailed: return "raster::Error::CanvasDrawingFailed";
            case Error::BlobExportFailed: return "raster::Error::BlobExportFailed";
            default: assert(false && "Invalid error code [raster::ToCStr()]");
        }
        return nullptr; // unreachable, need to suppress compiler warning
    }

    inline const char *ToCStr(const Format fmt) {
        switch (fmt) {
            case Format::Png: return "png";
            case Format::Jpeg: return "jpeg";
            case Format::Webp: return "webp";
            case Format::Unknown: return "unknown";
            default: assert(false && "Invalid format code [raster::ToCStr()]");
        }
        return nullptr; // unreachable, need to suppress compiler warning
    }

    inline std::string GetImageHeader(const std::string_view img, const std::size_t pos,
                                      const std::size_t n) {
        std::ostringstream out;
        const auto begin = std::min(img.cbegin() + pos, img.cend());
        const auto end = std::min(img.cbegin() + pos + n, img.cend());
        assert(begin <= end && "Wrong header range [raster::GetImageHeader()]");
        std::for_each(begin, end,
                      [&out](std::uint8_t ch) {
                          out << std::format("{:02X}", ch) << ' ';
                      });
        std::string header = out.str();
        if (not header.empty()) { header.pop_back(); }  // removes the last space
        return header;
    }

    inline Format GetImageFormat(const std::string_view img) {
        const char* png_sig = "89 50 4E 47 0D 0A 1A 0A"; // ASCII ? 'PNG' ...
        const char* jpeg_sig = "FF D8";
        const char* riff_sig = "52 49 46 46"; // ASCII 'RIFF'
        const char* webp_sig = "57 45 42 50"; // ASCII 'WEBP'
        if (GetImageHeader(img, 0, 8) == png_sig)
            return Format::Png;
        if (GetImageHeader(img, 0, 2) == jpeg_sig)
            return Format::Jpeg;
        if (GetImageHeader(img, 0, 4) == riff_sig
            and GetImageHeader(img, 8, 4) == webp_sig)
            return Format::Webp;
        // Other
        return Format::Unknown;
    }
}

#endif // EMSCRIPTEN_SVG2IMG_H
