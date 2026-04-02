#include "RaylibRenderAdapter.h"
#include "raylib.h"

static Color ToRaylibColor(const Color4& c) {
    return Color{ c.r, c.g, c.b, c.a };
}

bool RaylibRenderAdapter::Init(int width, int height, const std::string& title) {
    InitWindow(width, height, title.c_str());
    SetExitKey(KEY_NULL);
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

void RaylibRenderAdapter::DrawTriangle(const TransformComponent& transform, const Color4& color) {
    float size = 60.0f * transform.scale.x;

    Vector2 top{
        transform.position.x,
        transform.position.y - size
    };

    Vector2 left{
        transform.position.x - size,
        transform.position.y + size
    };

    Vector2 right{
        transform.position.x + size,
        transform.position.y + size
    };

    ::DrawTriangle(top, left, right, ToRaylibColor(color));
}

void RaylibRenderAdapter::DrawRectangle(const TransformComponent& transform, const Color4& color) {
    float width = 120.0f * transform.scale.x;
    float height = 80.0f * transform.scale.y;

    ::DrawRectangle(
        static_cast<int>(transform.position.x - width / 2.0f),
        static_cast<int>(transform.position.y - height / 2.0f),
        static_cast<int>(width),
        static_cast<int>(height),
        ToRaylibColor(color)
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