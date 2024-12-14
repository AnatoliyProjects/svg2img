// svg2img v. 1.0
// Header-only library for converting svg to raster images (png/jpeg/webp) via browser.
// Required Emscripten/WebAssembly environment.
// SPDX-FileCopyrightText: Copyright © 2024 Anatoly Petrov <petrov.projects@gmail.com>
// SPDX-License-Identifier: MIT

// Library usage example. Creates the Svg2Img Demo in the browser.
// Tech stack: Emscripten + Dear ImGui + GLFW + OpenGL + stb

#include <cstring>
#include <format>
#include <sstream>
#include <string_view>

#include "emscripten/em_js.h"
#include "emscripten/emscripten.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "svg2img.h"

// Global state and constants
static constexpr char example[] =
R"-(<svg width="300" height="300" xmlns="http://www.w3.org/2000/svg">
    <rect width="300" height="300" x="0" y="0" rx="40" ry="40" fill="lavender"/>
    <text x="20" y="50" fill="pink" stroke="blue" font-size="40" transform="rotate(35 40,50)">Svg2Img example!</text>
    <text x="150" y="150" font-size="40" transform="rotate(35 40,50)">&#129395;</text>
</svg>)-";
static constexpr ImVec4 bg = ImVec4(0.101f, 0.101f, 0.101f, 1.00f);
static GLFWwindow *glfw_window = nullptr;
static bool imgui_demo = true;

// Shows an alert message in the browser.
// We can't use Dear ImGui popups because we want to notify users about setup errors.
EM_JS(void, Alert, (const char* text), {
    const stext = UTF8ToString(text);
    alert(stext);
});

// GLFW error callback.
static void GlfwErrCb(int, const char *desc) { Alert(desc); }

bool LoadTextureFromMemory(const char *data, size_t size, GLuint *out_texture,
                           float *out_width, float *out_height);

// Draws Svg2Img Demo.
// You are here for this example!
void Svg2ImgDemo() {
    // State and helpers
    static bool inited = false;
    static constexpr size_t columns = 1024, rows = 10, buf_size = columns * rows;
    static char text[buf_size]{};
    static const char *all_formats[3] = {"image/png", "image/jpeg", "image/webp"};
    static int format_idx = 0;
    static float quality = 1.0f, x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f,
                 zoom = 1.0f;
    static float out_width = 0.0f, out_height = 0.0;
    static size_t out_size = 0;
    static const char *out_err = "RasterError::None";
    static std::string out_img;
    static GLuint texture = 0;

    // Sets default svg example as text.
    static auto default_text = [] { strncpy(text, example, buf_size); };

    // Clears the image and associated data.
    static auto clear_image = [] {
        out_width = 0.0f, out_height = 0.0f, out_size = 0;
        out_err = "RasterError::None";
        out_img.clear();
        if (texture) glDeleteTextures(1, &texture);
        texture = 0;
    };

    // Resets options to default values.
    static auto reset_opts = [] {
        format_idx = 0;
        quality = 1.0f, x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f, zoom = 0.0f;
    };

    // Callback for the raster::SvgToImage().
    // Thanks to the std::function<>, we may pass as a callback ordinary functions,
    // lambdas, functors, etc.
    static auto cb = [](const std::string_view img, const raster::Error err, void *) {
        if (static_cast<bool>(err)) {
            out_err = raster::ToCStr(err);
            Alert(std::format("Error occurs: {}", out_err).c_str());
            return;
        }
        // no error
        out_size = img.size();
        // Note that raster::SvgToImage() frees the image buffer after the callback returns.
        // Copy the image data if you want to use it further.
        out_img = img;
        LoadTextureFromMemory(img.data(), img.size(), &texture, &out_width, &out_height);
    };

    // UI
    if (not inited) {
        default_text();
        inited = true;
    }
    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::Begin("Svg2Img Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // Push spacing
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 15));

        // Text input
        ImGui::SeparatorText("Enter SVG here");
        ImGui::InputTextMultiline("##text", text, buf_size,
                                  ImVec2(850.0f, ImGui::GetTextLineHeight() * rows),
                                  ImGuiInputTextFlags_AllowTabInput);

        // Actions
        ImGui::SeparatorText("Actions");
        if (ImGui::Button("Convert to img")) {
            clear_image();
            raster::SvgToImage({text}, cb, nullptr, all_formats[format_idx],
                               quality, x, y, width, height, zoom);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear text")) { memset(text, 0, buf_size); }
        ImGui::SameLine();
        if (ImGui::Button("Default text")) { default_text(); }

        // Image
        ImGui::SeparatorText("Image");
        ImGui::Text("%s | Size (bytes) %zu | W %.3f | H %.3f",
                    out_err, out_size, out_width, out_height);
        const char * out_fmt = raster::ToCStr(raster::GetImageFormat(out_img));
        const std::string header = raster::GetImageHeader(out_img, 0, 12);
        ImGui::Text("Format %s | Header (12 bytes) %s", out_fmt, header.c_str());
        if (texture) {
            ImGui::Image(texture, ImVec2(out_width, out_height));
        } else {
            ImGui::TextUnformatted("No image to show!");
        }

        // Options
        ImGui::SeparatorText("Options");
        if (ImGui::Button("Reset Options")) { reset_opts(); }
        ImGui::Combo("format", &format_idx, all_formats, std::size(all_formats));
        ImGui::SliderFloat("quality", &quality, 0.0f, 1.0f);
        ImGui::InputFloat("x", &x, 10);
        ImGui::InputFloat("y", &y, 10);
        ImGui::InputFloat("width", &width, 10);
        ImGui::InputFloat("height", &height, 10);
        ImGui::InputFloat("zoom", &zoom, 0.25);

        // Pop spacing
        ImGui::PopStyleVar();
    }
    ImGui::End();
}

// Loads the raster image into an OpenGL texture.
// Based on https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples#example-for-opengl-users
// The original decision was created by Omar Cornut (https://github.com/ocornut) and other contributors (if present).
bool LoadTextureFromMemory(const char *data, const size_t size, GLuint *out_texture,
                           float *out_width, float *out_height) {
    // Load to memory
    int img_w = 0, img_h = 0;
    stbi_uc *img_data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(data),
                                              static_cast<int>(size),
                                              &img_w, &img_h, nullptr, 4);
    if (img_data == nullptr) {
        Alert("Failed to load image.");
        return false;
    }

    // Create an OpenGL texture identifier
    GLuint img_texture;
    glGenTextures(1, &img_texture);
    glBindTexture(GL_TEXTURE_2D, img_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_w, img_h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, img_data);

    // Release
    stbi_image_free(img_data);

    *out_texture = img_texture;
    *out_width = static_cast<float>(img_w);
    *out_height = static_cast<float>(img_h);

    return true;
}

// Renders a new frame.
// Based on https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl3/main.cpp
// The original decision was created by Omar Cornut (https://github.com/ocornut) and other contributors (if present).
void Frame() {
    // Poll and handle events (inputs, window resize, etc.)
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui demo
    if (imgui_demo) ImGui::ShowDemoWindow(&imgui_demo);
    // svg2img demo
    Svg2ImgDemo();

    // Render frame
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(glfw_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(bg.x * bg.w, bg.y * bg.w, bg.z * bg.w, bg.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(glfw_window);
}

// Entry point.
// Based on https://github.com/ocornut/imgui/blob/master/examples/example_glfw_opengl3/main.cpp
// The original decision was created by Omar Cornut (https://github.com/ocornut) and other contributors (if present).
int main(int, char **) {
    // Setup GLFW
    glfwSetErrorCallback(GlfwErrCb);
    if (!glfwInit()) {
        Alert("Unable to init GLFW.");
        return 1;
    }
    const char *glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfw_window = glfwCreateWindow(1280, 720, "Dear ImGui + svg2img example",
                                   nullptr, nullptr);
    if (glfw_window == nullptr) {
        Alert("GLFW window creation failed.");
        return 1;
    }
    glfwMakeContextCurrent(glfw_window);
    glfwSwapInterval(1);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup backends
    ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(glfw_window, "#canvas");
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Main loop
    emscripten_set_main_loop(Frame, 0, true);

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(glfw_window);
    glfwTerminate();

    return 0;
}
