#include "RaylibRenderAdapter.h"
#include "raylib.h"

bool RaylibRenderAdapter::Init(int width, int height, const std::string& title) {
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
    return true;
}

bool RaylibRenderAdapter::ShouldClose() const {
    return WindowShouldClose();
}

void RaylibRenderAdapter::BeginFrame() {
    BeginDrawing();
}

void RaylibRenderAdapter::Clear() {
    ClearBackground(RAYWHITE);
}

void RaylibRenderAdapter::DrawTriangle() {
    ::DrawTriangle(
        { 400.0f, 180.0f },
        { 300.0f, 380.0f },
        { 500.0f, 380.0f },
        RED
    );
}

void RaylibRenderAdapter::DrawTextOnScreen(const std::string& text, int x, int y, int fontSize) {
    DrawText(text.c_str(), x, y, fontSize, BLACK);
}

void RaylibRenderAdapter::EndFrame() {
    EndDrawing();
}

void RaylibRenderAdapter::Shutdown() {
    CloseWindow();
}