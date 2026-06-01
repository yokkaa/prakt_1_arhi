#pragma once
#include "RenderAdapter.h"

class RaylibRenderAdapter : public RenderAdapter {
public:
    bool Init(int width, int height, const std::string& title) override;
    bool ShouldClose() const override;

    void BeginFrame() override;
    void Clear() override;
    void SetCamera(const CameraComponent& camera) override;
    void Begin3D() override;
    void End3D() override;

    void DrawTriangle(const TransformComponent& transform, const Color4& color) override;
    void DrawRectangle(const TransformComponent& transform, const Color4& color) override;
    void DrawDebugAABB(const AABB& aabb, const Color4& color) override;
    void DrawDebugSphere(const Vec3& center, float radius, const Color4& color) override;
    void DrawTextOnScreen(const std::string& text, int x, int y, int fontSize) override;

    void DeleteShaderProgram(const GPUShaderProgram& shader) override;

    GPUTexture CreateTexture(const TextureData& data) override;
    GPUMesh UploadMesh(const MeshData& data) override;

    GPUShaderProgram CreateShaderProgram(
        const std::string& vertexSource,
        const std::string& fragmentSource
    ) override;

    void EndFrame() override;
    void Shutdown() override;

    void DrawMesh(
        const GPUMesh& mesh,
        const GPUTexture& texture,
        const GPUShaderProgram& shader,
        const TransformComponent& transform
    ) override;

private:
    CameraComponent activeCamera;
};
