#pragma once
#include "RenderAdapter.h"

class RaylibRenderAdapter : public RenderAdapter {
public:
    bool Init(int width, int height, const std::string& title) override;
    bool ShouldClose() const override;

    void BeginFrame() override;
    void Clear() override;
    void DrawTriangle() override;
    void DrawTextOnScreen(const std::string& text, int x, int y, int fontSize) override;
    void EndFrame() override;

    void Shutdown() override;
};
