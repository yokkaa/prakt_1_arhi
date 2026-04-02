#pragma once
#include <string>

class RenderAdapter {
public:
    virtual ~RenderAdapter() = default;

    virtual bool Init(int width, int height, const std::string& title) = 0;
    virtual bool ShouldClose() const = 0;

    virtual void BeginFrame() = 0;
    virtual void Clear() = 0;
    virtual void DrawTriangle() = 0;
    virtual void DrawTextOnScreen(const std::string& text, int x, int y, int fontSize) = 0;
    virtual void EndFrame() = 0;

    virtual void Shutdown() = 0;
};