#pragma once

#include <string>

#include "AABB.h"
#include "Components.h"
#include "Mesh.h"
#include "Texture.h"
#include "ShaderProgram.h"

class RenderAdapter {
public:
    virtual ~RenderAdapter() = default;

    virtual bool Init(int width, int height, const std::string& title) = 0;
    virtual bool ShouldClose() const = 0;

    virtual void BeginFrame() = 0;
    virtual void Clear() = 0;
    virtual void SetCamera(const CameraComponent& camera) = 0;
    virtual void Begin3D() = 0;
    virtual void End3D() = 0;

    virtual void DrawTriangle(const TransformComponent& transform, const Color4& color) = 0;
    virtual void DrawRectangle(const TransformComponent& transform, const Color4& color) = 0;
    virtual void DrawDebugAABB(const AABB& aabb, const Color4& color) = 0;
    virtual void DrawDebugSphere(const Vec3& center, float radius, const Color4& color) = 0;
    virtual void DrawTextOnScreen(const std::string& text, int x, int y, int fontSize) = 0;

    virtual GPUMesh UploadMesh(const MeshData& data) = 0;
    virtual GPUTexture CreateTexture(const TextureData& data) = 0;

    virtual GPUShaderProgram CreateShaderProgram(
        const std::string& vertexSource,
        const std::string& fragmentSource
    ) = 0;

    virtual void DeleteShaderProgram(const GPUShaderProgram& shader) = 0;

    virtual void DrawMesh(
        const GPUMesh& mesh,
        const GPUTexture& texture,
        const GPUShaderProgram& shader,
        const TransformComponent& transform
    ) = 0;

    virtual void EndFrame() = 0;
    virtual void Shutdown() = 0;
};